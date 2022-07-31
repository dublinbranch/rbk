#include "clickhouse.h"
#include "min_mysql.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/RAII//resetAfterUse.h"
#include "rbk/defines//stringDefine.h"
#include "rbk/filesystem//filefunction.h"
#include "rbk/thread//threadstatush.h"
#include <QDateTime>
#include <QDebug>

extern thread_local ThreadStatus::Status* localThreadStatus;
extern Runnable                           runnable;

ClickHouseException::ClickHouseException(const QString& _msg, uint skip)
    : ExceptionV2(_msg, skip){};

ClickHouse::ClickHouse(QString _url, const QString& _logFolder, CurlKeeper* _curl) {
	this->curl = _curl;
	url        = _url.toUtf8();
	curl_easy_setopt(this->curl->get(), CURLOPT_POST, 1);
	curl_easy_setopt(this->curl->get(), CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(this->curl->get(), CURLOPT_HEADER, 0);

	this->logFolder = _logFolder;
}

/*
 * to specify result format
 * https://clickhouse.com/docs/en/interfaces/formats/
 *
 * e.g.
 * select ... from ... where ... FORMAT CSVWithNames
 */
CurlCallResult ClickHouse::query(const QString& sql) {
	if (sql.isEmpty()) {
		return {};
	}
	auto post = sql.toUtf8();
	curl_easy_setopt(curl->get(), CURLOPT_POSTFIELDS, post.data());
	//Click house is normally very fast, we do batch aggregation with 1 second timeout
	curl_easy_setopt(curl->get(), CURLOPT_TIMEOUT_MS, 1500);

	ResetAfterUse reset1(localThreadStatus->state, ThreadState::ClickHouse);

	localThreadStatus->time.clickHouse.start();
	auto res = urlGetContent2(url, false, curl->get(), false);
	localThreadStatus->time.clickHouse.pause();
	if (res.httpCode != 200) {
		auto err = QSL("error executing clickHouse query:\n%1\nquery:\n%2")
		               .arg(res.packDbgMsg(true))
		               .arg(sql);
		throw ClickHouseException(err);
	}
	return res;
}

QString composeSql(const QString& table, const CHParam& param, bool extendedInsert) {
	if (table.isEmpty()) {
		return QString();
	}

	if (extendedInsert) {
		QString     sql = "INSERT INTO " + table + " SET \n";
		QStringList part;
		for (const auto& row : param) {
			part << row.first + "=" + row.second;
		}
		sql += part.join(",") + ";";
		return sql;
	} else {
		QString     sql = "INSERT INTO " + table + "\n(";
		QStringList key;
		QStringList value;
		for (const auto& row : param) {
			key << row.first;
			value << row.second;
		}
		sql += key.join(',') + ")\nVALUES\n(" + value.join(',') + ")";
		return sql;
	}
}

QString toIpV6ForClickHouse(const QString& ip) {
	QString res;
	if (ip.contains(':')) {
		// ip is v6
		// keep it unchanged (clickHouse accepts ipv6)
		res = QSL("'%1'").arg(ip);
	} else {
		// ip is v4
		// convert it to v6
		res = QSL("toIPv6('%1')").arg(ip);
	}
	return res;
}

void CHParam::insert(const char* q, const char* v) {
	insert(QString(q), QString(v));
}

void CHParam::insert(const QString& q, const QVariant& v) {
	if (auto s = v.toString(); !s.isEmpty()) {
		mapV2::insert({q, s});
	}
}

void CHParam::insert(const QString& q, const QString& v, Escape escape) {
	if (v.isEmpty()) {
		return;
	}
	switch (escape) {
	case none:
		mapV2::insert({q, v});
		return;

	case quote:
		mapV2::insert({q, QSL("'") + v + QSL("'")});
		return;

	case base64:
		mapV2::insert({q, base64this(v)});
		return;
	}
}

void CHParam::insert(const QString& q, const std::string v, Escape escape) {
	if (!v.empty()) {
		insert(q, QString::fromStdString(v), escape);
	}
}
