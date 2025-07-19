#include "mincurl.h"
#include "qstringtokenizer.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/RAII//resetAfterUse.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include "rbk/thread/threadstatush.h"
#include <QByteArray>
#include <QDebug>
#include <QString>
#include <boost/json.hpp>
#include <curl/curl.h>

extern thread_local ThreadStatus::Status* localThreadStatus;

/**
 * @brief QBReader
 * Used to debug sent data in HTTPS only site
 *
 *
        curl_easy_setopt(curl, CURLOPT_POST, true);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, s1.data.size());
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, QBReader);
        curl_easy_setopt(curl, CURLOPT_READDATA, &s1);
 *
 * @return
 */
size_t QBReader(char* ptr, size_t size, size_t nmemb, void* userdata) {
	(void)ptr;
	auto readMe = (QBReaderSt*)userdata;
	auto cur    = readMe->pos;
	readMe->pos = size * nmemb;

	size_t remnant = (size_t)readMe->data.size() - cur;

	auto sent = std::min(remnant, size * nmemb);

	ptr = readMe->data.data() + cur;

	return sent;
}

size_t QBWriter(void* contents, size_t size, size_t nmemb, QByteArray* userp) {
	size_t realsize = size * nmemb;

	auto rs = static_cast<int>(realsize);

	userp->append(static_cast<char*>(contents), rs);
	return realsize;
}

size_t QSWriter(void* contents, size_t size, size_t nmemb, QString* userp) {
	size_t realsize = size * nmemb;
	userp->append(static_cast<char*>(contents));
	return realsize;
}

size_t FakeCurlWriter(void* contents, size_t size, size_t nmemb, void* userp) {
	(void)contents;
	(void)userp;
	size_t realsize = size * nmemb;
	return realsize;
}

size_t STDWriter(void* contents, size_t size, size_t nmemb, std::string* userp) {
	size_t realsize = size * nmemb;
	userp->append(static_cast<char*>(contents), realsize);
	return realsize;
}

CURLTiming curlTimer(CURLTiming& timing, CURL* curl) {
	curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &timing.totalTime);
	curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD_T, &timing.speed);
	curl_easy_getinfo(curl, CURLINFO_NAMELOOKUP_TIME, &timing.dnsTime);
	curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &timing.connTime);
	curl_easy_getinfo(curl, CURLINFO_APPCONNECT_TIME, &timing.appConnect);
	curl_easy_getinfo(curl, CURLINFO_PRETRANSFER_TIME, &timing.preTransfer);
	curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &timing.startTtransfer);
	curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &timing.trxByte);
	return timing;
}

QString CURLTiming::print() const {
	auto line = QStringLiteral("total: %1 \t, sslReady: %2").arg(totalTime).arg(preTransfer);
	return line;
}

QString CURLTiming::print2() const {
	static const QString skel = R"(    namelookup_time => %1
    connect_time => %2
    pretransfer_time => %3
    starttransfer_time => %4
    total_time => %5
)";
	return skel.arg(dnsTime).arg(connTime).arg(preTransfer).arg(startTtransfer).arg(totalTime);
}

CURLTiming::CURLTiming(CURL* curl) {
	curlTimer(*this, curl);
}

void CURLTiming::read(CURL* curl) {
	curlTimer(*this, curl);
}

CURLTiming curlTimer(CURL* curl) {
	CURLTiming timing;
	return curlTimer(timing, curl);
}

QByteArray urlGetContent(const QByteArray& url, bool quiet, CURL* curl) {
	auto res = urlGetContent2(url, quiet, curl);
	return res.result;
}

QByteArray urlGetContent(const QString& url, bool quiet, CURL* curl) {
	auto u = url.toUtf8();
	return urlGetContent(u, quiet, curl);
}

CurlHeader::~CurlHeader() {
	if (chunk && !used) {
		qCritical() << "you set header, but never connected! To avoid this error use the CurlHeader INSIDE the CurlKeeper! or manually set" << QStacker16Light();
	}
	clear();
}

void CurlHeader::add(const QByteAdt& header) {
	chunk = curl_slist_append(chunk, header);
	set();
}

void CurlHeader::clear() {
	if (chunk) {
		curl_slist_free_all(chunk);
		chunk = nullptr;
	}
	set();
}

const curl_slist* CurlHeader::getChunk() const {
	return chunk;
}

const curl_slist* CurlHeader::get() const {
	return chunk;
}

void CurlHeader::set(CurlKeeper& marx) {
	curl = &marx;
	set();
}

void CurlHeader::set() {
	if (curl) {
		curl_easy_setopt(curl->get(), CURLOPT_HTTPHEADER, get());
		used = true;
	}
}

CurlKeeper::CurlKeeper() {
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60); // default is 0 which is unlimited, not a good default
	header.curl = this;
}

CurlKeeper::~CurlKeeper() {
	// todo check usage of header or other stuff ?
	// in theory you should use CurlHeader
	curl_easy_cleanup(curl);
}

CurlKeeper::operator CURL*() const {
	return get();
}

CURL* CurlKeeper::get() const {
	return curl;
}

/**
  why this stuff is not built in inside curl is a mistery...
 * @brief parseHeader
 * @param headers
 * @return
 */
[[nodiscard]] Header parseHeader(const QString& headers) {
	Header header;
	auto   lines = QStringTokenizer{headers, u"\r\n"};
	//auto   c     = lines.toContainer();
	// qDebug() << c;
	for (auto& line : lines) {
		// QString c = line.toString(); //debug symbol are broken for stringview -.-
		if (line.length() > 0) {
			auto found = line.indexOf(u":");
			if (found > 0) {
				auto value = line.mid(found + 2);
				auto key   = line.left(found);
				// auto k      = key.toString();
				header.insert({key.toString(), value.toString()});
			}
		}
	}

	return header;
}

CurlCallResult urlPostContent(const QByteAdt& url, const QByteAdt& post, bool quiet, CURL* curl) {
	CurlCallResult result;
	char           errbuf[CURL_ERROR_SIZE] = {0};
	CURL*          useMe                   = curl;
	result.url                             = url;
	if (!useMe) {
		useMe = curl_easy_init();
		curl_easy_setopt(useMe, CURLOPT_TIMEOUT, 60); // 1 minute, if you do not like use you own curl
	} else {
	}

	if (post.isEmpty()) {
		curl_easy_setopt(useMe, CURLOPT_CUSTOMREQUEST, "POST");
	} else {
		curl_easy_setopt(useMe, CURLOPT_POSTFIELDS, post.constData());
	}

	// all those are needed
	curl_easy_setopt(useMe, CURLOPT_URL, url.constData());

	curl_easy_setopt(useMe, CURLOPT_WRITEDATA, &result.result);
	curl_easy_setopt(useMe, CURLOPT_WRITEFUNCTION, QBWriter);

	curl_easy_setopt(useMe, CURLOPT_HEADERFUNCTION, QSWriter);
	curl_easy_setopt(useMe, CURLOPT_HEADERDATA, &result.headerRaw);

	curl_easy_setopt(useMe, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(useMe, CURLOPT_SSL_VERIFYHOST, 0);

	curl_easy_setopt(useMe, CURLOPT_ERRORBUFFER, errbuf);

	result.errorCode = curl_easy_perform(useMe);
	result.errorMsg  = errbuf;
	curlTimer(result.timing, useMe);
	curl_easy_getinfo(useMe, CURLINFO_RESPONSE_CODE, &result.httpCode);
	if (result.errorCode == CURLE_OK) {
		result.ok     = true;
		result.header = parseHeader(result.headerRaw);
	} else if (!quiet) {
		qCritical().noquote() << "For:" << url << " code " << result.errorCode << "\n " << errbuf;
	}

	if (!curl) { // IF a local instance was used
		curl_easy_cleanup(useMe);
	}

	return result;
}

CurlCallResult urlPutContent(const QByteAdt& url, const QByteAdt& put, CurlKeeper& curl, bool quiet) {
	CurlCallResult result;
	char           errbuf[CURL_ERROR_SIZE] = {0};
	CURL*          useMe                   = curl;

	curl_easy_setopt(useMe, CURLOPT_CUSTOMREQUEST, "PUT");
	curl_easy_setopt(useMe, CURLOPT_POSTFIELDS, put.constData());

	// all those are needed
	curl_easy_setopt(useMe, CURLOPT_URL, url.constData());

	curl_easy_setopt(useMe, CURLOPT_WRITEDATA, &result.result);
	curl_easy_setopt(useMe, CURLOPT_WRITEFUNCTION, QBWriter);

	curl_easy_setopt(useMe, CURLOPT_HEADERFUNCTION, QSWriter);
	curl_easy_setopt(useMe, CURLOPT_HEADERDATA, &result.headerRaw);

	curl_easy_setopt(useMe, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(useMe, CURLOPT_SSL_VERIFYHOST, 0);

	curl_easy_setopt(useMe, CURLOPT_ERRORBUFFER, errbuf);

	result.errorCode = curl_easy_perform(useMe);
	result.errorMsg  = errbuf;
	curlTimer(result.timing, useMe);
	if (result.errorCode == CURLE_OK) {
		result.ok     = true;
		result.header = parseHeader(result.headerRaw);
	} else if (!quiet) {
		qCritical().noquote() << "For:" << url << " code " << result.errorCode << "\n " << errbuf;
	}

	return result;
}

CurlCallResult urlGetContent2(const QByteAdt& url, bool quiet, CURL* curl, bool LTS) {
	QElapsedTimer timer;
	if (LTS) {
		ResetOnExit reset1(localThreadStatus->state, ThreadState::cURL);
		timer.start();
	}

	CurlCallResult result;
	char           errbuf[CURL_ERROR_SIZE] = {0};
	CURL*          useMe                   = curl;

	CurlKeeper curlK;
	if (!useMe) {
		useMe = curlK;
		curl_easy_setopt(useMe, CURLOPT_TIMEOUT, 60); // 1 minute, if you do not like use you own curl
	}
	if (!url.isEmpty()) {
		curl_easy_setopt(useMe, CURLOPT_URL, url.constData());
		result.url = url;
	}

	curl_easy_setopt(useMe, CURLOPT_WRITEDATA, &result.result);
	curl_easy_setopt(useMe, CURLOPT_WRITEFUNCTION, QBWriter);

	curl_easy_setopt(useMe, CURLOPT_HEADERFUNCTION, QSWriter);
	curl_easy_setopt(useMe, CURLOPT_HEADERDATA, &result.headerRaw);

	curl_easy_setopt(useMe, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(useMe, CURLOPT_SSL_VERIFYHOST, 0);

	curl_easy_setopt(useMe, CURLOPT_ERRORBUFFER, errbuf);

	result.errorCode = curl_easy_perform(useMe);
	result.errorMsg  = errbuf;
	curlTimer(result.timing, useMe);
	if (result.errorCode == CURLE_OK) {
		{
			char* redirectUrl = nullptr;
			curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &redirectUrl);
			if (redirectUrl) {
				result.redirectUrl = redirectUrl;
			}
		}
		result.ok = true;

		char* ip;
		if (CURLcode res = curl_easy_getinfo(useMe, CURLINFO_PRIMARY_IP, &ip); res == CURLE_OK && ip) {
			result.ip.fromLocal8Bit(ip);
		} else {
			result.ip = "127.0.0.1";
		}

		curl_easy_getinfo(useMe, CURLINFO_RESPONSE_CODE, &result.httpCode);
		result.header = parseHeader(result.headerRaw);
	} else if (!quiet) {
		qDebug().noquote() << "For:" << url << "\n " << curl_easy_strerror(result.errorCode) << "\n"
		                   << errbuf << QStacker16Light();
	}

	if (LTS) {
		localThreadStatus->time.addCurlTime(timer.nsecsElapsed());
	}

	return result;
}

bool CaseInsensitiveCompare::operator()(QStringView a, QStringView b) const noexcept {
	// we have to provide the operator<=
	return a.compare(b, Qt::CaseInsensitive) < 0;
}

CurlCallResult::CurlCallResult() {
	// quite hard to have smaller header nowadays
	headerRaw.reserve(512);
}

QString CurlCallResult::getError() const {
	return curl_easy_strerror(errorCode);
}

QString CurlCallResult::packDbgMsg(bool extraInfo) const {
	static const std::string skel = R"(
Url: {}
Ip: {} | Response: {}
Header: {}
Timing: {}
{}
)";

	QString extraInfoValue;
	if (extraInfo) {
		static const std::string extraInfoSkel = R"(errorCode: {}
errorMsg: {}
result: {})";
		extraInfoValue                         = F16(extraInfoSkel, asString(errorCode), errorMsg, result.data());
	}

	auto msg = F16(skel, url, ip, httpCode, header.serialize(), timing.print2(), extraInfoValue);
	return msg;
}

CurlForm::CurlForm(CURL* _curl) {
	//set a flag on curlkeeper to wait for the form to be bound ?
	this->curl = _curl;
	/* Create the form */
	form = curl_mime_init(curl);
}

curl_mime* CurlForm::get() const {
	return form;
}

void CurlForm::add(const QString& name, const QString& value) {
	add(name.toStdString(), value.toStdString());
}

void CurlForm::add(const QByteArray& name, const QByteArray& value) {
	add(name.toStdString(), value.toStdString());
}

void CurlForm::add(const std::string_view& name, const std::string_view& value) {
	curl_mimepart* field = nullptr;
	field                = curl_mime_addpart(form);
	//The name string is copied into the part, thus the associated storage may safely be released or reused after call.
	curl_mime_name(field, name.data());
	curl_mime_data(field, value.data(), CURL_ZERO_TERMINATED);
	asJson(name, value);
}

void CurlForm::addFile(const std::string& name, const std::string& path) {
	curl_mimepart* field = nullptr;
	field                = curl_mime_addpart(form);
	curl_mime_filedata(field, path.c_str());
	curl_mime_name(field, name.c_str());
}

void CurlForm::connect() const {
	//FIRSTTIMER add a check if destructor is called and the form is not used.
	curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
}

void CurlForm::asJson(const std::string_view& name, const std::string_view& value) const {
	if (!saveJson) {
		return;
	}
	saveJson->insert_or_assign(name, value);
}

CurlForm::~CurlForm() {
	curl_mime_free(form);
}

CurlForm::operator curl_mime*() const {
	return form;
}

QString Header::serialize() const {
	QStringList list;
	for (auto [key, value] : *this) {
		auto term = QSL("%1=%2").arg(key, value);
		list.append(term);
	}
	return list.join("\n");
}

CurlCallResult urlPutContent(const QByteAdt& url, const QByteAdt& post, bool quiet) {
	CurlKeeper curl;
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60); // 1 minute, if you do not like use you own curl
	return urlPutContent(url, post, curl, quiet);
}
