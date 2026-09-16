#ifndef SQLROWV2_H
#define SQLROWV2_H

#include "mytype.h"
#include "rbk/defines/stringDefine.h"
#include "rbk/mapExtensor/mapV2.h"
#include "rbk/mapExtensor/missingkeyex.h"
#include "rbk/misc/swapType.h"
#include "rbk/number/intTypes.h"
#include "rbk/serialization/QDataStreamer.h"
#include "rbk/string/comparator.h"
#include "rbk/string/concept.h"
#include <QVector>
#include <fmt/format.h>
#include <rbk/QStacker/exceptionv2.h>
#include <type_traits>
#include <utility>

class DB;
class sqlResult;
class sqlRow;

namespace SqlResV2 {
struct Field {
	MyType type;
	uint   pos;
};

using TypeMap = mapV2<std::string, Field, std::less<>>;
} // namespace SqlResV2

//using for key std string has many advantage at the moment compared to qbarray, which is still better for type conversion
//
// Adapting an older codebase (sqlRow / get2 / getIfNotNull / == "NULL"):
//
// SQL NULL used to be the cell *text* "NULL" (S_SQL_NULL). That cannot tell a
// NULL cmmsToken from a client that sends the four letters NULL as its token.
// fetchResultV2 now sets nulls[i] from MYSQL_ROW == nullptr. getIf / rqIf /
// isNull test that flag; a stored string "NULL" is then a string.
//
// rq / get still map the sentinel ("NULL" → "" / 0) so existing call sites
// keep working. Do not extend old sqlRow.
//
// Fetch with queryV2 / queryCacheV2 / queryCacheLineV2. Do not wrap
// SqlRowV2(oldRow) in a loop: the converting ctor infers NULL from the text
// and allocates a TypeMap per row. A one-off wrap of a non-secret row is OK.
// queryCacheV2 writes cachedSQL_V3_* (V2 files have no nulls vector).
class SqlRowV2 {

      public:
	SqlRowV2() = default;
	SqlRowV2(const sqlRow& old);

	//std:string has the massive advantage of SSO... and is quite easy to create on the fly a no copy QByteArray if we need conversion
	QVector<std::string> data;
	QVector<bool>        nulls; // nulls[i] == true  →  data[i] was SQL NULL

	std::shared_ptr<SqlResV2::TypeMap> columns = nullptr;
	[[nodiscard]] bool                 empty() const;

	// Friend declaration for serialization
	friend QDataStream& operator<<(QDataStream& out, const SqlRowV2& row);

	// Friend declaration for deserialization
	friend QDataStream& operator>>(QDataStream& in, SqlRowV2& row);

	std::string prettyPrint(DB* db) const;

	enum class GetReason : u8 {
		ok,       // dest assigned
		notFound, // key not in the column map (getIf only; rqIf throws)
		null,     // cell is SQL NULL
		zero,     // {.skip0 = true} and converted value == 0
		empty,    // {.skipEmpty = true} and converted string is empty
	};

	struct GetRes {
		GetReason reason = GetReason::notFound;
		explicit operator bool() const {
			return reason == GetReason::ok;
		}
	};

	struct GetIf {
		bool skip0     = false; // arithmetic 0 counts as unset (bool false too)
		bool skipEmpty = false; // empty std::string / QString counts as unset
	};

	// Default to rqIf: a missing key is a typo / stale SELECT. A LEFT JOIN
	// column is present with a NULL cell, not missing. getIf is for SELECT *
	// across schema versions. dest is untouched unless reason == ok.
	//
	//   // was: for (auto& old : db->queryCache2(sql, ttl)) { SqlRowV2 row(old); ... }
	//   for (auto& row : db->queryCacheV2(sql, ttl)) { ... }
	//   auto row = db->queryCacheLineV2(sql, ttl, true);
	//
	//   // was: if (row.get2<string>("jobId") != "NULL" && !jobId.empty())
	//   if (row.rqIf("jobId", hr.jobId, {.skip0 = true})) { /* real id */ }
	//
	//   row.rqIf("idOffset", wm.idOffset); // NULL → false, dest unchanged
	//
	//   // was: if (token.empty() || token == "NULL")
	//   if (auto r = row.rqIf("cmmsToken", wm.cmmsToken, {.skipEmpty = true}); !r) {
	//       err = r.reason == GetReason::null ? "no token assigned"
	//                                        : "token is blank";
	//   }
	//
	//   std::string title;
	//   if (!row.rqIf("title", title, {.skipEmpty = true})) {
	//       title = row.rq("titleR1") + " " + row.rq("titleR2");
	//   }
	//
	//   if (row.getIf("optionalCol", v)) { /* present in this schema */ }
	//   if (row.isNull("jobId")) { /* LEFT JOIN miss */ }
	//
	// Designated initializers follow declaration order:
	// {.skip0 = true} or {.skip0 = true, .skipEmpty = true}. The reverse
	// does not compile. skip0 is arithmetic only; skipEmpty is string / QString.

	struct Founded {
		const std::string* val   = nullptr;
		bool               found = false;

		explicit operator bool() const {
			return found;
		}
	};

	template <StdStringable Key>
	int fpOpt(Key key) const {
		// queryCacheLineV2(required = false) returns a default row with no column map
		if (!columns) {
			return -1;
		}
		if (auto iter = columns->find(key); iter != columns->end()) {
			return (int)iter->second.pos;
		}
		return -1;
	}

	/*
	if(auto v = row.get("x"); v){
	        val = *v.value;
	}
	*/
	template <StdStringLike Key>
	[[nodiscard]] auto get(Key k) const {
		if (auto pos = fpOpt(k); pos > -1) {
			return Founded{&data[pos], true};
		}
		return Founded();
	}

	/*
	Type value;
	bool found = map.get("key",value);
	*/
	template <StdStringable Key, class Value>
	bool get(Key k, Value& v) const {
		if (auto pos = fpOpt(k); pos > -1) {
			if constexpr (std::is_same_v<Value, std::string>) {
				//swapType already maps the NULL sentinel to 0 for every numeric type, string was
				//the only one letting it through as text, so it ended up written back into the DB.
				//Use getIf / isNull to tell SQL NULL apart from an empty string or the text "NULL".
				v = (data[pos] == S_SQL_NULL) ? std::string() : data[pos];
			} else {
				try {
					swapType(data[pos], v);
				} catch (ExceptionV2& e) {
					e.msg += fmt::format("\n For key {}", k);
				}
			}
			return true;
		}
		return false;
	}

	template <StdStringable Key, class Value>
	void rq(Key k, Value& v) const {
		if constexpr (std::same_as<Key, const char*>) {
			if (!get(std::string_view(k), v)) {
				throw MissingKeyEX(fmt::format("Key not found in row: {}", k));
			}
		} else {
			if (!get(k, v)) {
				throw MissingKeyEX(fmt::format("Key not found in row: {}", k));
			}
		}
	}

	template <class Value, StdStringable Key>
	Value rq(Key k) const {
		Value v;
		// Convert const char* to string_view if needed
		if constexpr (std::same_as<Key, const char*>) {
			if (!get(std::string_view(k), v)) {
				throw MissingKeyEX(fmt::format("Key not found in row: {}", k));
			}
		} else {
			if (!get(k, v)) {
				throw MissingKeyEX(fmt::format("Key not found in row: {}", k));
			}
		}
		return v;
	}

	template <StdStringable Key>
	std::string rq(Key k) const {
		std::string v;
		if constexpr (std::same_as<Key, const char*>) {
			if (!get(std::string_view(k), v)) {
				throw MissingKeyEX(fmt::format("Key not found in row: {}", k));
			}
		} else {
			if (!get(k, v)) {
				throw MissingKeyEX(fmt::format("Key not found in row: {}", k));
			}
		}
		return v;
	}

	// Missing key → notFound (no throw). NULL / skip0 / skipEmpty leave dest unchanged.
	template <StdStringable Key, class Value>
	GetRes getIf(Key k, Value& dest, GetIf opt = {}) const {
		if constexpr (std::same_as<Key, const char*>) {
			return getIf(std::string_view(k), dest, opt);
		} else {
			const auto pos = fpOpt(k);
			if (pos < 0) {
				return GetRes{GetReason::notFound};
			}
			if (pos < static_cast<int>(nulls.size()) && nulls[pos]) {
				return GetRes{GetReason::null};
			}
			Value tmp{};
			try {
				swapType(data[pos], tmp);
			} catch (ExceptionV2& e) {
				e.msg += fmt::format("\n For key {}", k);
				throw;
			}
			if (opt.skip0) {
				if constexpr (std::is_arithmetic_v<Value>) {
					if (tmp == Value{0}) {
						return GetRes{GetReason::zero};
					}
				}
			}
			if (opt.skipEmpty) {
				if constexpr (std::is_same_v<Value, std::string>) {
					if (tmp.empty()) {
						return GetRes{GetReason::empty};
					}
				} else if constexpr (std::is_same_v<Value, QString>) {
					if (tmp.isEmpty()) {
						return GetRes{GetReason::empty};
					}
				}
			}
			dest = std::move(tmp);
			return GetRes{GetReason::ok};
		}
	}

	// Like getIf, but a missing key throws MissingKeyEX (same as rq).
	template <StdStringable Key, class Value>
	GetRes rqIf(Key k, Value& dest, GetIf opt = {}) const {
		auto r = getIf(k, dest, opt);
		if (r.reason == GetReason::notFound) {
			throw MissingKeyEX(fmt::format("Key not found in row: {}", k));
		}
		return r;
	}

	// Tests nulls[pos], never the text "NULL". Missing key throws MissingKeyEX.
	template <StdStringable Key>
	bool isNull(Key k) const {
		if constexpr (std::same_as<Key, const char*>) {
			return isNull(std::string_view(k));
		} else {
			const auto pos = fpOpt(k);
			if (pos < 0) {
				throw MissingKeyEX(fmt::format("Key not found in row: {}", k));
			}
			return pos < static_cast<int>(nulls.size()) && nulls[pos];
		}
	}

	void replace(std::string key, std::string newValue) {
		if (auto pos = fpOpt(key); pos > -1) {
			data[pos] = newValue;
			if (pos >= nulls.size()) {
				nulls.resize(data.size());
			}
			nulls[pos] = false;
		} else {
			throw MissingKeyEX(fmt::format("Key not found : {}", key));
		}
	}
};

class SqlResultV2 : public QVector<SqlRowV2> {
      public:
	friend class DB;
	SqlResultV2();
	// Infers NULL from BSQL_NULL text, same as SqlRowV2(const sqlRow&).
	// Prefer queryV2 / queryCacheV2 so nulls[] comes from MYSQL_ROW.
	SqlResultV2(const sqlResult& old);
	// Friend declaration for serialization
	friend QDataStream& operator<<(QDataStream& out, const SqlResultV2& result);

	// Friend declaration for deserialization
	friend QDataStream& operator>>(QDataStream& in, SqlResultV2& result);

	bool    fromCache = false;
	QString toString();

	std::shared_ptr<SqlResV2::TypeMap> columns;
};
#endif // SQLROWV2_H
