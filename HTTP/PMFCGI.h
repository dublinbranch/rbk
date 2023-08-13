#ifndef PMFCGI_H
#define PMFCGI_H

#include "url.h"
#include <QString>
#include <QStringList>
#include <map>
#include <rbk/mapExtensor/mapV2.h>
#include <string>

struct SKQ : public QString {
	mutable bool skip = false;
};

class bHeaders : public mapV2<QString, SKQ> {
      public:
	void        add(const std::string_view& key, const std::string_view& value);
	void        set(const std::multimap<std::string, std::string>& headers);
	std::string serialize(const QStringList& skipHeaders, bool initialTab, bool skipEmptyHeaders) const;
};

struct PMFCGI {
	std::string remoteIp;
	bHeaders    headers;
	bHeaders    cookies;

	//This is only the path, and in nginx X we do the intermediate routing
	std::string path;
	std::string body;
	//remember not automatically decoded! //TODO extend and decode on request ?
	multiMapV2<QString, QString> post;
	QueryParams                  get;
	multiMapV2<QString, QString> request;
	//server name, if needed must be forwarded by nginx else will be 127.0.0.1 from the header

	void        decodeGet();
	void        extractCookies();
	std::string serializeMsg(const QByteArray& msg, bool light = false) const;
	std::string serializeMsg(const char* msg) const;
	std::string serializeMsg(const QString& msg) const;
	std::string serialize() const;
	std::string serializeBase() const;

	/*
	 * 0 ->	debug not active
	 * 1 ->	active in standard mode:
	 *		everything printed in standard output (terminal where this server is running) or logged is also sent to
	 *		the client that performed the request
	 */
	enum DebugMode {
		none       = 0, //not active...
		printAll   = 1, //active in standard mode: (terminal where this server is running) or logged is also sent to the client that performed the request
		writeCHSQL = 2  //force write on log the clickhouse sql
	};

	uint debug = 0;
};

void requestBeging();
void requestEnd();
void registerFlushTime();

multiMapV2<QString, QString> decodePost(const std::string& form);

#endif // PMFCGI_H
