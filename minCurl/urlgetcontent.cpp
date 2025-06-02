#include "urlgetcontent.h"
#include "errorlog.h"
#include "mincurl.h"
#include <QDebug>

// timeOut in seconds
UrlGetContent::UrlGetContent(const QByteArray& _url, bool _quiet, int _category, int _timeOut, CURL* _curl) {
	this->url      = _url;
	this->quiet    = _quiet;
	this->category = _category;
	this->curl     = _curl;
	this->timeOut  = _timeOut;
}

QByteArray UrlGetContent::execute(ErrorLog* eLog) {
	QByteArray response;
	CURL*      useMe = curl;
	if (!useMe) {
		useMe = curl_easy_init();
		curl_easy_setopt(useMe, CURLOPT_TIMEOUT, timeOut);
		//99.9% of the time is what we want
		curl_easy_setopt(useMe, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(useMe, CURLOPT_SSL_VERIFYHOST, 0);
	}
	char errbuf[CURL_ERROR_SIZE] = {0};

	//Nothing cames to my mind that will ever change those 3
	curl_easy_setopt(useMe, CURLOPT_URL, url.constData());
	curl_easy_setopt(useMe, CURLOPT_WRITEFUNCTION, QBWriter);
	curl_easy_setopt(useMe, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(useMe, CURLOPT_ERRORBUFFER, errbuf);

	for (uint i = 0; i < retryNum; ++i) {
		curlCode      = curl_easy_perform(useMe);
		callPerformed = true;

		if (curlCode != CURLE_OK && !quiet) {
			qDebug().noquote() << "For:" << url << "\n " << errbuf;
		}

		if (eLog) {
			CurlCall call;
			call.response = response;
			call.curl     = useMe;
			call.get      = url;
			call.category = category;
			call.curlCode = curlCode;
			memccpy(call.errbuf, errbuf, CURL_ERROR_SIZE, 1);

			sql = eLog->logQuery(&call);
		}

		if (curlCode == CURLE_OK) {
			break;
		}
	}
	if (curlCode != CURLE_OK && !quiet) {
		qWarning() << "max number (" << retryNum << ") of curl calls failed for " << url;
	}

	// "ok" is for testing
	[[maybe_unused]] auto ok = curl_easy_getinfo(useMe, CURLINFO_RESPONSE_CODE, &httpCode);
	if (!curl) { //IF a local instance was used
		curl_easy_cleanup(useMe);
	}

	return response;
}

bool UrlGetContent::curlOk() const {
	if (callPerformed && curlCode == CURLcode::CURLE_OK) {
		return true;
	}
	return false;
}

CURLcode UrlGetContent::getCurlCode() const {
	return curlCode;
}
