#include "errorlog.h"
#include "rbk/misc/b64.h"
#include <QDateTime>
#include <QDebug>

QString ErrorLog::logQuery(const curlCall* call) {
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

	switch (format) {
	case Format::sql: {
		static const QString skel = R"EOD(
		INSERT INTO %1.%2 SET
		ts = %3,
		totalTime = %4,
		preTransfer = %5,
		curlCode = %6,
		httpCode = %7,
		get = %8,
		post = %9,
		response = %10,
		errBuf = %11,
		category = %12,
		headers = %13;
	)EOD";
		auto                 log  = skel.arg(db)
		               .arg(table)
		               .arg(now)
		               .arg(totalTime)
		               .arg(preTransfer)
		               .arg(call->curlCode)
		               .arg(httpCode)
					   .arg(base64this(get))
					   .arg(base64this(post))
					   .arg(base64this(truncatedResp))
					   .arg(base64this(sErrBuf))
					   .arg(call->category)
					   .arg(base64this(header.serialize()));
		logList.append(log);
		return log;
	} break;
	case Format::csv: {
		QStringList robe;
		robe << QString::number(now);
		robe << QString::number(totalTime);
		robe << QString::number(preTransfer);
		robe << QString::number(call->curlCode);
		robe << QString::number(httpCode);
		robe << header.serialize();
		robe << sErrBuf;
		robe << get;
		robe << post;
		robe << truncatedResp;

		auto log = robe.join("\t,\t");
		logList.append(log);
		return log;
	} break;
	}
	return QString();
}
