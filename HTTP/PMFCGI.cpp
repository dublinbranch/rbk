#include "PMFCGI.h"
#include "Payload.h"
#include "beastConfig.h"
#include "rbk/QStacker/httpexception.h"
#include "rbk/defines/stringDefine.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/misc/b64.h"
#include "url.h"
#include <QDateTime>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

using namespace std;

void bHeaders::add(const std::string_view& key, const std::string_view& value) {
	QStringAdt k = key;
	QStringAdt v = value;

	//we no longer have the problem of having orrible traffic ATM, so we can skip this
	// if (!isValidUTF8(key, &k)) {
	// 	throw ExceptionV2(QSL("Invalid utf8 in header %1 (value %2)").arg(base64this(key), QString::fromStdString(value.data())));
	// }
	// if (!isValidUTF8(value, &v)) {
	// 	throw ExceptionV2(QSL("Invalid utf8 in header %1 (value %2)").arg(QString::fromStdString(key.data()), base64this(value)));
	// }

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
	string    buffer;
	qsizetype longest = 0;
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

		longest = max(longest, (qsizetype)key.size());
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

void PMFCGI::decodeGet() {
	get                  = url.query;
	get.notFoundCallback = HttpException::HttpParamErrorHandler1;
}

void PMFCGI::decodePost() {
	//the :: mean use the public function not the one in this class with the same name
	post = ::decodePost(body);
}

void PMFCGI::extractCookies() {
	cookies = bHeaders();
	if (auto v = headers.get("cookie"); v) {
		for (auto& ck : v.val->split(';')) {
			auto dp = ck.split('=');
			if (dp.size() != 2) {
				throw HttpException(QSL("invalid cookie %1").arg(ck));
			}
			cookies->insert({dp[0].trimmed(), {dp[1].trimmed()}});
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

string PMFCGI::getBasePath() const {
	if (conf->basePathFunctor) {
		return conf->basePathFunctor(this);
	}

	if (!curBasePath.empty()) {
		return curBasePath;
	}

	if (conf->basePath.empty()) {
		if (conf->port == 80) {
			curBasePath = F("http://{}/", localIp);
		} else {
			curBasePath = F("http://{}:{}/", localIp, conf->port);
		}
	} else {
		curBasePath = conf->basePath;
	}
	return curBasePath;
}

void Payload::setStandardHeaders(bool addCors) {
	headers.insert({"Expires", "Sun, 01 Jan 2014 00:00:00 GMT"});
	headers.insert({"Cache-Control", "no-store, no-cache, must-revalidate"});
	headers.insert({"Cache-Control", "post-check=0, pre-check=0"});
	headers.insert({"Pragma", "no-cache"});
	if (addCors) {
		headers.insert({"Access-Control-Allow-Origin", "*"});
	}
	statusCode = 200;
}

void Payload::setCacheHeader(uint ttl) {
	if (ttl > 0) {
		headers.insert({"cache-control", "public, max-age=" + std::to_string(ttl)});
	}
}

void Payload::redirect(const StringAdt& location) {
	headers.insert({"Location", location});
	statusCode = 302;
}

void Payload::setCookie(const std::string_view& key, const StringAdt& value, u32 cookieTTL, bool sameSite, bool secure) {
	headers.setCookie(key, value, cookieTTL, sameSite, secure);
}

multiMapV2<QString, QString> decodePost(const std::string& form) {
	auto copy = QString::fromStdString(form);
	return decodePost(copy);
}

multiMapV2<QString, QString> decodePost(const QString& form) {
	if (form.isEmpty()) {
		return {};
	}
	//if is a json it will be handled by the actual function using it
	if (form.at(0) == '{') {
		return {};
	}
	multiMapV2<QString, QString> res;
	auto                         rows = form.split('&');

	for (auto& row : rows) {
		auto pair = row.split('=');
		if (pair.size() != 2) {
			if (rows.size() == 1) {
				//in some cases we send ONLY a single JSON back and not the post format stuff...
				// in this case is fine to just return empty and use a custom decoder
				return {};
			}
			throw HttpException("invalid line in post" + row);
		}
		auto c0 = pair[0].toUtf8();
		c0.replace("+", " ");

		auto c1 = pair[1].toUtf8();
		//for REASON fromPercentEncoding does not decode the + to space...
		c1.replace("+", " ");

		auto t0 = QByteArray::fromPercentEncoding(c0);
		auto t1 = QByteArray::fromPercentEncoding(c1);

		res.insert({t0, t1});
	}
	return res;
}

void Post::checkIfUnused(bool) const {
	if (this->empty()) {
		return;
	}
	throw HttpException(F("Unused parameter found! {}", fmt::join(this->getAllKeys(), ";")));
}
