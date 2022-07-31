#ifndef ERRORLOG_H
#define ERRORLOG_H

#include "mincurl.h"
#include <QString>
#include <QStringList>
#include <curl/curl.h>

struct curlCall {
	CURL*      curl = nullptr;
	CURLcode   curlCode;
	QByteArray response;
	QByteArray get;
	QByteArray post;
	char       errbuf[CURL_ERROR_SIZE] = {0};
	int        category                = 0;
	CURLTiming timing;
};

class ErrorLog {
      public:
	QString     logQuery(const curlCall* call);
	QStringList logList{};
	QString     db                      = "set me";
	QString     table                   = "set me";
	int         truncatedResponseLength = 100;
	enum Format {
		sql,
		csv
	};
	Format format = Format::sql;
};

#endif // ERRORLOG_H
