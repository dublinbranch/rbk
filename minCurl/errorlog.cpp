#include "errorlog.h"
#include "rbk/minMysql/min_mysql.h"
#include "rbk/minMysql/sqlcomposer.h"
#include "rbk/misc/b64.h"
#include <QDateTime>
#include <QDebug>

ErrorLog::ErrorLog(DB* db_, CurlCall* call_) {
	conn = db_;
	call = call_;
}

std::string ErrorLog::logQuery() {
	auto curl     = call->curl;
	auto response = call->response;
	auto get      = call->get;

	QByteArray post;
	if (call->post.isEmpty()) {
		post = "NULL";
	} else {
		post = call->post;
	}

	// milliseconds
	double     now = static_cast<double>(QDateTime::currentMSecsSinceEpoch()) / 1000.0;
	CURLTiming timing(curl);
	// seconds
	double totalTime   = timing.totalTime;
	double preTransfer = timing.preTransfer;

	long httpCode = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

	QByteArray truncatedResp;
	if (truncatedResponseLength > 0) {
		truncatedResp = response.left(truncatedResponseLength);
	} else {
		truncatedResp = response;
	}

	QByteArray sErrBuf;
	if (call->errbuf[0] == '\0') {
		sErrBuf = "NULL";
	} else {
		sErrBuf = call->errbuf;
	}

	SqlComposer c(conn);
	c.setTable(db, table);
	c.push("ts", now);
	c.push("accountId", accountId);
	c.push("retry", attempt);
	c.push("totalTime", totalTime);
	c.push("preTransfer", preTransfer);
	c.push("curlCode", call->curlCode);
	c.push("httpCode", httpCode);
	c.push("get", get);
	c.push("post", post);
	c.push("response", truncatedResp);
	c.push("errBuf", sErrBuf);
	c.push("category", call->category);
	c.push("headers", header.serialize());

	return c.composeInsert();
}
