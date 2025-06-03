#ifndef ERRORLOG_H
#define ERRORLOG_H

#include "mincurl.h"
#include <QString>
#include <QStringList>
#include <curl/curl.h>

struct CurlCall {
	CURL*      curl = nullptr;
	CURLcode   curlCode;
	QByteArray response;
	QByteArray get;
	QByteArray post;
	char       errbuf[CURL_ERROR_SIZE] = {0};
	int        category                = 0;
	CURLTiming timing;
};

class DB;

class ErrorLog {
      public:
	ErrorLog(DB* db_, CurlCall* call_);
	~ErrorLog();

	std::string db        = "set me";
	std::string table     = "set me";
	int         attempt   = 0;
	u64         accountId = 0;
	Header      header;
	int         truncatedResponseLength = 100;

      private:
	std::string composeLogQuery();
	DB*         conn = nullptr;
	CurlCall*   call;
};

#endif // ERRORLOG_H
