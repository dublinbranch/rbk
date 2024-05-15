#ifndef PMFCGI_H
#define PMFCGI_H

#include "url.h"
#include <QString>
#include <QStringList>
#include <map>
#include <rbk/mapExtensor/mapV2.h>
#include <string>

class BeastConf;

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
      public:
	const BeastConf*        conf = nullptr;
	std::string             remoteIp;
	std::string             localIp;
	bHeaders                headers;
	std::optional<bHeaders> cookies;

	Url url;
	/**
	 * This is only the path, and in nginx X we do the intermediate routing
	 * YES is quite annoing that it has a / at the beginning, but .. just clip with
	 * .substr(1)
	 */
	std::string path;
	std::string body;
	//remember not automatically decoded! //TODO extend and decode on request ?
	multiMapV2<QString, QString> post;
	QueryParams                  get;
	multiMapV2<QString, QString> request;
	//server name, if needed must be forwarded by nginx else will be 127.0.0.1 from the header

	void        decodeGet();
	void        decodePost();
	void        extractCookies();
	std::string serializeMsg(const QByteArray& msg, bool light = false) const;
	std::string serializeMsg(const char* msg) const;
	std::string serializeMsg(const QString& msg) const;
	std::string serialize() const;
	std::string serializeBase() const;

	//return the base path of the server
	std::string getBasePath() const;

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
	//can be changed by the getBasePath call
	mutable std::string curBasePath;

      private:
};

multiMapV2<QString, QString> decodePost(const QString& form);
multiMapV2<QString, QString> decodePost(const std::string& form);

#endif // PMFCGI_H
