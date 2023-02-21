#ifndef CLICKHOUSE_H
#define CLICKHOUSE_H

#include "rbk/mapExtensor/mapV2.h"
#include "rbk/minCurl/mincurl.h"
#include <QByteArray>
#include <QString>
#include <QVariant>
#include <concepts>

class ClickHouse {
      public:
	explicit ClickHouse(QString _url, const QString& _logFolder, CurlKeeper* _curl);
	CurlCallResult query(const QString& sql);

      private:
	QByteArray  url;
	CurlKeeper* curl = nullptr;
	QString     logFolder;
};

//TODO this can probably be used also for MYSQL, rename and move in a separated header
//FIXME this is partiall duplicating and extending SqlComposer merge the 2 ?
class CHParam : public mapV2<QString, QString> {
      public:
	enum Escape {
		none,
		quote,
		base64
	};
	//This is mostly to avoid error when inserting 0 and beeing converted to false
	template <std::integral T>
	void insert(const QString& q, const T& v) {
		mapV2::insert({q, QString::number(v)});
	}

	void insert(const char* q, const char* v);

	void insert(const QString& q, const QVariant& v);

	void insert(const QString& q, const QString& v, Escape escape = Escape::base64);

	void    insert(const QString& q, const std::string v, Escape escape = Escape::base64);
	QString composeSql(const std::string& table, bool extendedInsert = false);
	QString composeSql(const QString& table, bool extendedInsert = false);
};

QString toIpV6ForClickHouse(const QString& ip);

#endif // CLICKHOUSE_H
