#ifndef SQLROWV2_H
#define SQLROWV2_H

#include "rbk/minMysql/sqlresult.h"
#include "rbk/serialization/QDataStreamer.h"
#include "rbk/string/comparator.h"
#include "rbk/string/concept.h"

namespace SqlResV2 {
struct Field {
	MyType type;
	uint   pos;
};

using TypeMap = mapV2<std::string, Field, std::less<>>;
} // namespace SqlResV2

//using for key std string has many advantage at the moment compared to qbarray, which is still better for type conversion
class SqlRowV2 {

      public:
	SqlRowV2() = default;
	SqlRowV2(const sqlRow& old);

	//std:string has the massive advantage of SSO... and is quite easy to create on the fly a no copy QByteArray if we need conversion
	QVector<std::string> data;

	std::shared_ptr<SqlResV2::TypeMap> columns = nullptr;
	[[nodiscard]] bool                 empty() const;

	// Friend declaration for serialization
	friend QDataStream& operator<<(QDataStream& out, const SqlRowV2& row);

	// Friend declaration for deserialization
	friend QDataStream& operator>>(QDataStream& in, SqlRowV2& row);

	struct Founded {
		const std::string* val   = nullptr;
		bool               found = false;

		explicit operator bool() const {
			return found;
		}
	};

	template <StdStringable Key>
	int fpOpt(Key key) const {
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
				v = data[pos];
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
};

class SqlResultV2 : public QVector<SqlRowV2> {
      public:
	friend class DB;
	SqlResultV2();
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
