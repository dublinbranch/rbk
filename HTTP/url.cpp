#include "url.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/fmtExtra/customformatter.h"
#include <QDebug>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

using namespace std;

std::string Url::prettyPrint() const {
	string buffer = full.toStdString() + "\n";
	//sooooo coool
	qsizetype longest = 0;
	for (auto& q : query) {
		longest = max(longest, (qsizetype)q.first.size());
	}
	for (auto& [key, value] : query) {
		buffer += fmt::format("  |--> {:<{}} : {}\n", key.toStdString(), longest, value);
	}
	return buffer;
}

QString Url::get3lvl() const {
	return getNlvl(3);
}

QString Url::get2lvl() const {
	return getNlvl(2);
}

QString Url::getNlvl(int pos) const {
	//for now let's start assuming we do not have nonsense in the url
	auto path = url.host().split('.');
	auto s    = path.size();
	if (s < pos) {
		return QString();
	}

	return path.at(s - pos);
}

QString QueryParams::join() const {
	QStringList list;
	for (auto& [key, value] : *this) {
		auto term = QSL("%1=%2").arg(key, value);
		list.append(term);
	}
	return list.join("&");
}

std::optional<string> QueryParams::checkIfUnused() const {
	if (this->empty()) {
		return {};
	}

	return "Unused parameter found! " + join().toStdString();
}

void QueryParams::setQuery(const QString& val) {
	QUrlQuery p;
	p.setQuery(val);

	//Must keep internal parameter encoded! to avoid splitting in the wrong position when you have annidated encoded stuff
	for (auto& [key, value] : p.queryItems(QUrl::FullyEncoded)) {
		if (value.isEmpty()) {
			//as people are dumb and keep adding useless stuff, which is also dangerour as some of those param can go in conflict so we forcefully remove them
			continue;
		}
		auto decoded = QByteArray::fromPercentEncoding(value.toUtf8());
		insert({key, decoded});
	}
}

QString QueryParams::get64(const QString& key) const {
	QString value;
	get64(key, value);
	return value;
}

/*
 * find "key" in this QueryParams, if present then save its decoded (base64) value in "value"
 */
bool QueryParams::get64(const QString& key, QString& value) const {

	if (auto v = get(key); v) {
		value = QString::fromUtf8(QByteArray::fromBase64(v.val->toUtf8(), QByteArray::Base64UrlEncoding));
		return true;
	}
	return false;
}

Url::Url(const std::string& _url, bool fix) {
	*this = Url(QString::fromStdString(_url), fix);
}

Url::Url(const QString& _url, bool fix) {
	set(_url, fix);
}

Url::Url(const QByteArray& _url, bool fix) {
	set(_url, fix);
}

void Url::set(const std::string& _url, bool fix) {
	set(QString::fromStdString(_url), fix);
}

void Url::set(const QString& _url, bool fix) {
	if (_url.isEmpty()) {
		return;
	}

	if (fix) {
		this->full = fixBrokenUrl(_url);
	} else {
		this->full = _url;
	}

	url.setUrl(this->full);
	if (!url.isValid()) {
		auto err = QSL("invalid url %1").arg(_url);

		//TODO
		// 1 - runnable globale è thread local, non dovrebbe essere condiviso per tutti i thread?
		// stesso errore su 10 threads mi spamma 10 messaggi
		// io voglio solo 1 e poi aspettare 5 min

		// 2 - Runnable è thread safe?
		// in generale come controllare se elemento è thread safe?

		//		if (runnable(err, 5 * secondsInMinute)) {
		//			qWarning().noquote() << err << QStacker16Light();
		//		}

		throw ExceptionV2(err);
	}

	//do not use FullyDecoded here! else the other encoded parameter will be decoded and lost in fullUrl and put in the main one
	auto sQuery = url.query(QUrl::FullyEncoded);
	query.setQuery(sQuery);
}

Url::DS Url::getDomain_SubDomain() {
	if (!ds.domain.isEmpty()) {
		return ds;
	}
	static bool        once = true;
	static QStringList notAllowed;
	if (once) {
		once = false;
		//List of nation to be skipped so seek.de.site.com will became seek.site.com
		//notAllowed = getMarketCodes();
		notAllowed.append("www");
	}

	static const QStringList semiTLD = {"co", "com", "edu", "asn", "org", "id", "net", "ac", "info", "uk"};

	auto host = url.host();
	// in CbsTk, when we have the form-data format, the host is a null ip. in this case domain and subDomain are not defined
	if (host.isEmpty() or (host == QSL("0.0.0.0"))) {
		return {};
	}
	auto part = host.split('.');
	if (part.size() < 2) {
		throw HttpException(QSL("invalid url %1").arg(full));
	}
	//remove the TLD (.com .net .org)
	auto TLD = part.takeLast();

	auto lastPart = part.last();
	if (semiTLD.contains(lastPart)) {
		//remove also the fake tld
		TLD = part.takeLast() + "." + TLD;
	}

	switch (part.size()) {
	case 0:
		throw HttpException(QSL("invalid url %1").arg(full));
	case 1:
		ds = {host, QString()};
		break;
	default: {
		auto domain = part.takeLast() + "." + TLD;
		//this is to handle when we have
		//     de.site.com
		//seek.de.site.com
		//de.seek.site.com
		QString sub;
		for (auto& p : part) {
			if (notAllowed.contains(p)) {
				continue;
			}
			sub = p + "." + domain;
		}
		ds = {domain, sub};
	}
	}
	return ds;
}

QString Url::getDomain() {
	auto domain = getDomain_SubDomain().domain;
	return domain;
}

QString Url::getSubDomain() {
	auto subDomain = getDomain_SubDomain().subDomain;
	return subDomain;
}

QString fixBrokenUrl(QString broken) {
	if (broken.startsWith("//")) { //this is not a valid URL this is a file path (more or less)
		broken.prepend("http:");
	} else if (!broken.startsWith("http")) {
		broken.prepend("http://"); //small fix
	}

	return broken;
}
