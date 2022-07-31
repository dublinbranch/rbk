#ifndef URLGETCONTENT_H
#define URLGETCONTENT_H

#include "errorlog.h"
#include <QByteArray>
#include <QString>
#include <curl/curl.h>

class UrlGetContent {
	  public:
	long httpCode = 0;

	UrlGetContent(const QByteArray& _url, bool _quiet, int _category, int _timeOut = 60, CURL* _curl = nullptr);
	QByteArray execute(ErrorLog* eLog = nullptr);
	QString    sql;

	uint retryNum = 1;
	bool curlOk() const;

	CURLcode getCurlCode() const;

	  private:
	QByteArray url;
	bool       quiet    = false;
	int        category = 0;
	int        timeOut  = 0;
	CURLcode   curlCode;
	bool       callPerformed = false;
	CURL*      curl          = nullptr;
};

#endif // URLGETCONTENT_H
