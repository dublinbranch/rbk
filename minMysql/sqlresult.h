#ifndef SQLRESULT_H
#define SQLRESULT_H

#include "rbk/mapExtensor/mapV2.h"
#include "sqlRow.h"
#include <mysql/mysql.h>

class MyType {
      public:
	//MyTypes() = default;
	MyType(enum_field_types& t);
	enum_field_types type;
	bool             isNumeric() const;
	bool             isFloat() const;
};

class sqlResult : public QList<sqlRow> {
      public:
	//Metadata for the column Type
	//TODO is not always SET, so make private anche check on access
	//FIXME also on CACHE HIT those are absent!
	mapV2<QByteArray, MyType> types;

	bool    fromCache = false;
	QString toString();
};

QString asString(const sqlRow& row);

#endif // SQLRESULT_H
