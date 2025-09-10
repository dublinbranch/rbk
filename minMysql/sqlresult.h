#ifndef SQLRESULT_H
#define SQLRESULT_H

#include "mytype.h"
#include "rbk/mapExtensor/mapV2.h"
#include "sqlRow.h"
class SqlResultV2;

class sqlResult : public QList<sqlRow> {
      public:
	friend class SqlResultV2;
	friend class DB;
	bool    fromCache = false;
	QString toString(bool noQuote = false);

	const mapV2<QByteArray, MyType>& getTypes() const;

      protected:
	mapV2<QByteArray, MyType> types;

      private:
	//Metadata for the column Type
	//TODO is not always SET, so make private and check on access
	//FIXME also on CACHE HIT those are absent!
};

QString asString(const sqlRow& row, bool noQuote = false);

#endif // SQLRESULT_H
