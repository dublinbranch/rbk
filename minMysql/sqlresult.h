#ifndef SQLRESULT_H
#define SQLRESULT_H

#include "mytype.h"
#include "rbk/mapExtensor/mapV2.h"
#include "sqlRow.h"

class sqlResult : public QList<sqlRow> {
      public:
	friend class DB;
	bool    fromCache = false;
	QString toString(bool noQuote = false);

	const mapV2<QByteArray, MyType>& getTypes() const;

      private:
	//Metadata for the column Type
	//TODO is not always SET, so make private and check on access
	//FIXME also on CACHE HIT those are absent!

	mapV2<QByteArray, MyType> types;
};

QString asString(const sqlRow& row, bool noQuote = false);

#endif // SQLRESULT_H
