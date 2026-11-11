#include "mincurl.h"
#include "qstringtokenizer.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/RAII//resetAfterUse.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include "rbk/thread/threadstatush.h"
#include <QByteArray>
#include <QDebug>
#include <QElapsedTimer>
#include <QString>
#include <boost/json.hpp>
#include <curl/curl.h>
#include <optional>

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
	// curl does not NUL-terminate the data it passes; use its length
	userp->append(QString::fromUtf8(static_cast<const char*>(contents), static_cast<int>(realsize)));
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
		qCritical().noquote() << "you set header, but never connected! To avoid this error use the CurlHeader INSIDE the CurlKeeper! or manually set" << QStacker16Light();
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

namespace {

enum class CurlMethod {
	get,
	post,
	put
};

const char* methodName(CurlMethod m) {
	switch (m) {
	case CurlMethod::get:
		return "GET";
	case CurlMethod::post:
		return "POST";
	case CurlMethod::put:
		return "PUT";
	}
	return "?";
}

/**
 * Undoes, on scope exit, what one call put on a handle that outlives it: the pointers to this call's own
 * data (error buffer, result buffers, body) and the temporary header list. A later call or a caller's own
 * curl_easy_perform on that handle must not write into a dead stack frame.
 */
class CallScope : private NoCopy {
      public:
	CURL*             h           = nullptr;
	bool              borrowed    = false;
	bool              bodySet     = false;
	curl_slist*       tempHeaders = nullptr;
	const curl_slist* baseHeaders = nullptr;

	~CallScope() {
		if (tempHeaders) {
			curl_easy_setopt(h, CURLOPT_HTTPHEADER, baseHeaders);
			curl_slist_free_all(tempHeaders);
		}
		if (!borrowed) {
			return; // own handle, cleaned up right after this
		}
		curl_easy_setopt(h, CURLOPT_ERRORBUFFER, static_cast<char*>(nullptr));
		curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, static_cast<curl_write_callback>(nullptr));
		curl_easy_setopt(h, CURLOPT_WRITEDATA, stdout); // curl's default
		curl_easy_setopt(h, CURLOPT_HEADERFUNCTION, static_cast<curl_write_callback>(nullptr));
		curl_easy_setopt(h, CURLOPT_HEADERDATA, static_cast<void*>(nullptr));
		if (bodySet) {
			static const char empty[] = "";
			curl_easy_setopt(h, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(-1));
			curl_easy_setopt(h, CURLOPT_POSTFIELDS, empty);
		}
	}
};

/**
 * The one transfer behind urlGetV2 / urlPostV2 / urlPutV2 and the v1 calls.
 *
 * v1 = true keeps the v1 rules, so the v1 callers see no change:
 * - a borrowed handle keeps its own timeouts;
 * - GET sets no method: minMysql/clickhouse.cpp sets POSTFIELDS on its handle and posts through
 *   urlGetContent2;
 * - GET with an empty url keeps the url the caller set on its handle;
 * - POST with an empty body is a bare CUSTOMREQUEST "POST"; CUSTOMREQUEST is never reset;
 * - the log lines keep their v1 wording.
 * Fixed for both: POSTFIELDSIZE is set (a binary body was cut at the first NUL byte), result.ip is filled,
 * httpCode is read for PUT too, the thread status really is "cURL" during the call, and no pointer to this
 * call's data is left on a borrowed handle.
 *
 * v1Handle: the raw handle a v1 call borrows. v2 borrows opt.curl.
 */
CurlCallResult curlPerform(CurlMethod method, const QByteAdt& url, const QByteAdt& body, const CurlOpt& opt, bool v1,
                           CURL* v1Handle = nullptr) {
	QElapsedTimer            timer;
	ResetOnExit<ThreadState> threadState;
	if (opt.LTS) {
		threadState.set(localThreadStatus->state, ThreadState::cURL);
		timer.start();
	}

	CurlCallResult result;
	char           errbuf[CURL_ERROR_SIZE] = {0};

	CURL* const borrowed = v1 ? v1Handle : (opt.curl ? opt.curl->get() : nullptr);

	std::optional<CurlKeeper> own;
	CURL*                     h = borrowed;
	if (!h) {
		own.emplace();
		h = own->get();
	}
	// Declared after `own`, so it runs before the own handle is cleaned up.
	CallScope scope;
	scope.h        = h;
	scope.borrowed = borrowed != nullptr;

	if (!v1 || !borrowed) {
		curl_easy_setopt(h, CURLOPT_TIMEOUT, opt.timeoutSec);
		curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, opt.connectTimeoutSec);
	}

	if (!(v1 && method == CurlMethod::get && url.isEmpty())) {
		curl_easy_setopt(h, CURLOPT_URL, url.constData());
		result.url = url;
	}

	bool sendBody = method != CurlMethod::get;
	if (v1) {
		if (method == CurlMethod::post && body.isEmpty()) {
			curl_easy_setopt(h, CURLOPT_CUSTOMREQUEST, "POST");
			sendBody = false;
		} else if (method == CurlMethod::put) {
			curl_easy_setopt(h, CURLOPT_CUSTOMREQUEST, "PUT");
		}
	} else {
		// Set the method on every call: a reused handle remembers the last one.
		curl_easy_setopt(h, CURLOPT_NOBODY, 0L);
		curl_easy_setopt(h, CURLOPT_CUSTOMREQUEST, method == CurlMethod::put ? "PUT" : static_cast<const char*>(nullptr));
		if (method == CurlMethod::get) {
			curl_easy_setopt(h, CURLOPT_HTTPGET, 1L);
		}
	}
	if (sendBody) {
		curl_easy_setopt(h, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size()));
		curl_easy_setopt(h, CURLOPT_POSTFIELDS, body.constData());
		scope.bodySet = true;
	}

	curl_easy_setopt(h, CURLOPT_WRITEDATA, &result.result);
	curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, QBWriter);
	curl_easy_setopt(h, CURLOPT_HEADERFUNCTION, QSWriter);
	curl_easy_setopt(h, CURLOPT_HEADERDATA, &result.headerRaw);
	curl_easy_setopt(h, CURLOPT_ERRORBUFFER, errbuf);

	curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, opt.verifyTls ? 1L : 0L);
	curl_easy_setopt(h, CURLOPT_SSL_VERIFYHOST, opt.verifyTls ? 2L : 0L);
	if (!opt.caFile.empty()) {
		curl_easy_setopt(h, CURLOPT_CAINFO, opt.caFile.c_str());
	}

	if (!opt.headers.empty()) {
		const curl_slist* base = opt.curl ? opt.curl->header.get() : nullptr;
		curl_slist*       list = nullptr;
		for (auto* it = base; it; it = it->next) {
			list = curl_slist_append(list, it->data);
		}
		for (const auto& line : opt.headers) {
			list = curl_slist_append(list, line.c_str());
		}
		scope.tempHeaders = list;
		scope.baseHeaders = base;
		curl_easy_setopt(h, CURLOPT_HTTPHEADER, list);
	}

	result.errorCode = curl_easy_perform(h);
	result.errorMsg  = errbuf;
	// With verification off curl still writes the verify result into errbuf. It is not an error then.
	if (!opt.verifyTls &&
	    result.errorMsg.contains("SSL certificate OpenSSL verify result: self-signed certificate in certificate chain")) {
		result.errorMsg.clear();
	}
	curlTimer(result.timing, h);
	curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &result.httpCode);

	if (result.errorCode == CURLE_OK) {
		result.header = parseHeader(result.headerRaw);

		char* effective = nullptr;
		if (curl_easy_getinfo(h, CURLINFO_EFFECTIVE_URL, &effective) == CURLE_OK && effective) {
			result.redirectUrl = effective;
		}
		char* ip = nullptr;
		if (curl_easy_getinfo(h, CURLINFO_PRIMARY_IP, &ip) == CURLE_OK && ip) {
			result.ip = QString::fromLocal8Bit(ip);
		} else {
			result.ip = "127.0.0.1";
		}
	} else if (!v1 && result.errorMsg.isEmpty()) {
		result.errorMsg = curl_easy_strerror(result.errorCode);
	}

	result.ok = result.errorCode == CURLE_OK && (!opt.okNeeds2xx || (result.httpCode >= 200 && result.httpCode < 300));

	if (result.errorCode != CURLE_OK && !opt.quiet) {
		if (v1 && method == CurlMethod::get) {
			qDebug().noquote() << "For:" << url << "\n " << curl_easy_strerror(result.errorCode) << "\n"
			                   << errbuf << QStacker16Light();
		} else if (v1) {
			qCritical().noquote() << "For:" << url << " code " << result.errorCode << "\n " << errbuf;
		} else {
			qCritical().noquote() << methodName(method) << url << "failed:" << curl_easy_strerror(result.errorCode)
			                      << "(" << result.errorCode << ")" << result.errorMsg;
		}
	}

	if (opt.LTS) {
		localThreadStatus->time.addCurlTime(timer.nsecsElapsed());
	}
	return result;
}

CurlOpt v1Opt(bool quiet, bool LTS) {
	CurlOpt opt;
	opt.verifyTls = false;
	opt.quiet     = quiet;
	opt.LTS       = LTS;
	return opt;
}

} // namespace

CurlCallResult urlGetV2(const QByteAdt& url, const CurlOpt& opt) {
	return curlPerform(CurlMethod::get, url, QByteAdt{}, opt, false);
}

CurlCallResult urlPostV2(const QByteAdt& url, const QByteAdt& body, const CurlOpt& opt) {
	return curlPerform(CurlMethod::post, url, body, opt, false);
}

CurlCallResult urlPutV2(const QByteAdt& url, const QByteAdt& body, const CurlOpt& opt) {
	return curlPerform(CurlMethod::put, url, body, opt, false);
}

CurlCallResult urlPostContent(const QByteAdt& url, const QByteAdt& post, bool quiet, CURL* curl) {
	auto result = curlPerform(CurlMethod::post, url, post, v1Opt(quiet, false), true, curl);
	result.redirectUrl.clear(); // v1 POST never filled it
	return result;
}

CurlCallResult urlPutContent(const QByteAdt& url, const QByteAdt& put, CurlKeeper& curl, bool quiet) {
	auto result = curlPerform(CurlMethod::put, url, put, v1Opt(quiet, false), true, curl.get());
	result.redirectUrl.clear(); // v1 PUT never filled it
	return result;
}

CurlCallResult urlGetContent2(const QByteAdt& url, bool quiet, CURL* curl, bool LTS) {
	auto result = curlPerform(CurlMethod::get, url, QByteAdt{}, v1Opt(quiet, LTS), true, curl);
	// v1 read the redirect from the caller's handle only, and the response code on success only.
	if (!curl) {
		result.redirectUrl.clear();
	}
	if (!result.ok) {
		result.httpCode = 0;
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

CurlCallResult urlGetCached(const QByteAdt& url, const QByteAdt fileName, uint ttl, CURL* curl) {
	CurlCallResult res;
	//max age of 0 means do not use cache, (but in the context of fileGetContents means ignore expired)
	if (ttl) {
		//Quiet is of course true as we do not even expect to have this file
		if (auto file = fileGetContents2(fileName, true, ttl); file) {
			res.result    = file.content;
			res.fromCache = true;
			res.ok        = true;
			return res;
		}
	}
	res = urlGetContent2(url, true, curl);
	filePutContents(res.result, fileName);
	res.fromCache = false;
	return res;
}
