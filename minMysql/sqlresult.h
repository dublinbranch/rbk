#ifndef SQLRESULT_H
#define SQLRESULT_H

#include "rbk/mapExtensor/mapV2.h"
#include "sqlRow.h"
#include <mysql/mysql.h>

class MyType {
      public:
	MyType() = default;
	MyType(const enum_field_types& t);
	enum_field_types type;
	bool             isNumeric() const;
	bool             isFloat() const;

	// Friend declaration for serialization
	friend QDataStream& operator<<(QDataStream& out, const MyType& myType) {
		// Serialize enum_field_types as its underlying integer type
		out << static_cast<int>(myType.type);
		return out;
	}

	// Friend declaration for deserialization
	friend QDataStream& operator>>(QDataStream& in, MyType& myType) {
		// Deserialize enum_field_types as an integer and cast back to enum
		int typeAsInt;
		in >> typeAsInt;
		myType.type = static_cast<enum_field_types>(typeAsInt);
		return in;
	}
};

class sqlResult : public QList<sqlRow> {
      public:
	friend class DB;
	bool    fromCache = false;
	QString toString();

	const mapV2<QByteArray, MyType>& getTypes() const;

      private:
	//Metadata for the column Type
	//TODO is not always SET, so make private and check on access
	//FIXME also on CACHE HIT those are absent!

	mapV2<QByteArray, MyType> types;
};

QString asString(const sqlRow& row);

#endif // SQLRESULT_H
