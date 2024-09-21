#ifndef SQLROWV2_H
#define SQLROWV2_H

#include "rbk/minMysql/sqlresult.h"
#include "rbk/serialization/QDataStreamer.h"
#include "rbk/string/comparator.h"

namespace SqlResV2 {
struct Field {
	MyType type;
	uint   pos;

	// Friend declaration for serialization
	friend QDataStream& operator<<(QDataStream& out, const Field& field) {
		// Serialize each member
		out << field.type;
		out << field.pos;
		return out;
	}

	// Friend declaration for deserialization
	friend QDataStream& operator>>(QDataStream& in, Field& field) {
		// Deserialize each member
		in >> field.type;
		in >> field.pos;
		return in;
	}
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
	friend QDataStream& operator<<(QDataStream& out, const SqlRowV2& row) {
		out << row.data;
		return out;
	}

	// Friend declaration for deserialization
	friend QDataStream& operator>>(QDataStream& in, SqlRowV2& row) {
		// Clear the map first to prepare for deserialization
		row.data.clear();
		in >> row.data;
		return in;
	}

	struct Founded {
		const QByteArray* val   = nullptr;
		bool              found = false;

		explicit operator bool() const {
			return found;
		}
	};

	template <class Key>
	int fpOpt(const Key& key) const {
		if (auto iter = columns->find(key); iter != columns->end()) {
			return iter->second.pos;
		}
		return -1;
	}

	/*
	if(auto v = row.get("x"); v){
	        val = *v.value;
	}
	*/
	template <class Key>
	[[nodiscard]] auto get(const Key& k) const {
		if (auto pos = fpOpt(k); pos > -1) {
			return Founded{&data[pos], true};
		}
		return Founded();
	}

	/*
	Type value;
	bool found = map.get("key",value);
	*/
	template <class Key, class Value>
	bool get(const Key& k, Value& v) const {
		if (auto pos = fpOpt(k); pos > -1) {
			if constexpr (std::is_same_v<Value, QByteArray>) {
				v = data[pos];
			} else {
				swapType(data[pos], v);
			}
			return true;
		}
		return false;
	}

	template <class Key, class Value>
	void rq(const Key& k, Value& v) const {
		if (!get(k, v)) {
			throw MissingKeyEX(fmt::format("Key not found in row: {}", k));
		}
	}

	template <isEnum T, class Key>
	[[nodiscard]] T rqe(const Key& key) const {
		i64 temp = 0;
		rq(key, temp);
		T t2 = T(temp);
		return t2;
	}

	template <class Key, isEnum T>
	void rqe(const Key& key, T& t) const {
		i64 temp = 0;
		rq(key, temp);
		t = T(temp);
	}

	template <class Value, class Key>
	Value rq(const Key& k) const {
		Value v;
		if (!get(k, v)) {
			throw MissingKeyEX(fmt::format("Key not found in row: {}", k));
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
	friend QDataStream& operator<<(QDataStream& out, const SqlResultV2& result) {
		// Serialize QVector<SqlRowV2> (inherited part)
		out << static_cast<const QVector<SqlRowV2>&>(result);
		out << *result.columns;
		return out;
	}

	// Friend declaration for deserialization
	friend QDataStream& operator>>(QDataStream& in, SqlResultV2& result) {
		// Deserialize QVector<SqlRowV2> (inherited part)
		in >> static_cast<QVector<SqlRowV2>&>(result);

		// Deserialize additional members
		in >> *result.columns;
		result.fromCache = true;
		for (auto& row : result) {
			row.columns = result.columns;
		}
		return in;
	}

	bool    fromCache = false;
	QString toString();

	std::shared_ptr<SqlResV2::TypeMap> columns;
};

#endif // SQLROWV2_H
