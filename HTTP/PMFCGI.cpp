#include "PMFCGI.h"
#include "rbk/QStacker/httpexception.h"
#include "rbk/defines/stringDefine.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/misc/b64.h"
#include "rbk/string/UTF8Util.h"
#include "url.h"
#include <QDateTime>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

using namespace std;

void bHeaders::add(const std::string_view& key, const std::string_view& value) {
	QString k, v;

	if (!isValidUTF8(key, &k)) {
		throw ExceptionV2(QSL("Invalid utf8 in header %1 (value %2)").arg(base64this(key), QString::fromStdString(value.data())));
	}
	if (!isValidUTF8(value, &v)) {
		throw ExceptionV2(QSL("Invalid utf8 in header %1 (value %2)").arg(QString::fromStdString(key.data()), base64this(value)));
	}

	//https://stackoverflow.com/questions/64967098/how-to-get-nginx-to-add-a-header-without-converting-it-to-lowercase
	//In short headers are CASE INSENSITIVE (slow -.-) but HTTP2 requires them to be lowercase
	insert({k.toLower(), {v}});
}

void bHeaders::set(const std::multimap<std::string, std::string>& headers) {
	for (const auto& [key, value] : headers) {
		add(key, value);
	}
}

string bHeaders::serialize(const QStringList& skipHeaders, bool initialSpacer, bool skipEmptyHeaders) const {
	string buffer;
	int    longest = 0;
	for (auto& [key, value] : *this) {
		if (skipHeaders.contains(key)) {
			value.skip = true;
			continue;
		}

		if (skipEmptyHeaders and value.isEmpty()) {
			// no empty headers
			value.skip = true;
			continue;
		}

		longest = max(longest, key.size());
	}
	for (auto& [key, value] : *this) {
		if (value.skip) {
			continue;
		}
		if (initialSpacer) {
			buffer += fmt::format("  |--> {:<{}} : {}\n", key.toStdString(), longest, static_cast<QString>(value));
		} else {
			buffer += fmt::format("{:<{}} : {}\n", key.toStdString(), longest, static_cast<QString>(value));
		}
	}

	return buffer;
}

void PMFCGI::extractCookies() {
	if (auto v = headers.get("cookie"); v) {
		for (auto& ck : v.val->split(';')) {
			auto dp = ck.split('=');
			if (dp.size() != 2) {
				throw HttpException(QSL("invalid cookie %1").arg(ck));
			}
			cookies.insert({dp[0].trimmed(), {dp[1].trimmed()}});
		}
	}
}

std::string PMFCGI::serializeMsg(const QByteArray& msg, bool light) const {
	auto   ts = QDateTime::currentDateTime().toString(mysqlDateTimeFormat);
	string aft;
	if (light) {
		aft = serializeBase();
	} else {
		aft = serialize();
	}

	auto final = fmt::format("@ {} Error: {} \n Status: {}", ts, msg, aft);
	return final;
}

std::string PMFCGI::serializeMsg(const char* msg) const {
	return serializeMsg(QByteArray(msg));
}

std::string PMFCGI::serializeMsg(const QString& msg) const {
	return serializeMsg(msg.toUtf8());
}

string PMFCGI::serialize() const {
	//add ip
	//	auto res = fmt::format("Get:\n{}\nRefUrl: {}\nRefurl2: {}\nHeaders:\n{}\nCookies:\n{}",
	//	                       Url(path, false).prettyPrint(),
	//	                       dk.url.prettyPrint(),
	//	                       dk.refUrl.prettyPrint(),
	//	                       headers.serialize(conf().stdSkipHeader, true, true),
	//	                       cookies.serialize(QStringList(), true, true));
	//	return res;
	return {};
}

string PMFCGI::serializeBase() const {
	return "remote ip and plain text url and header should be enought";
}

#include "Payload.h"

void Payload::setStandardHeaders(bool addCors) {
	headers.insert({"Expires", "Sun, 01 Jan 2014 00:00:00 GMT"});
	headers.insert({"Cache-Control", "no-store, no-cache, must-revalidate"});
	headers.insert({"Cache-Control", "post-check=0, pre-check=0"});
	headers.insert({"Pragma", "no-cache"});
	if (addCors) {
		headers.insert({"Access-Control-Allow-Origin", "*"});
	}
}

multiMapV2<QString, QString> decodePost(const std::string& form) {
	multiMapV2<QString, QString> res;
	auto                         copy = QString::fromStdString(form);
	auto                         rows = copy.splitRef('&');
	for (auto& row : rows) {
		auto pair = row.split('=');
		if (pair.size() != 2) {
			throw HttpException("invalid line in post" + row);
		}
		res.insert({pair[0].toString(), pair[1].toString()});
	}
	return res;
}
