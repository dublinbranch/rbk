#pragma once

#include "curl/curl.h"
#include "rbk/BoostJson/fwd.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/mapExtensor/mapV2.h"
#include "rbk/mixin/NoCopy.h"
#include <QByteArray>
#include <QString>

using CURL = void;
class QString;
using size_t = unsigned long int;

struct CURLTiming {
	double     totalTime      = 0;
	double     dnsTime        = 0;
	double     connTime       = 0;
	double     appConnect     = 0;
	double     preTransfer    = 0;
	double     startTtransfer = 0;
	curl_off_t speed          = 0;
	curl_off_t trxByte        = 0;
	QString    print() const;
	QString    print2() const;
	CURLTiming() = default;
	CURLTiming(CURL* curl);
	void read(CURL* curl);
	void printTime(QString& response) const;
};
CURLTiming curlTimer(CURLTiming& timing, CURL* curl);

// In theory CURL will write to STDOUT if nothing defined
size_t FakeCurlWriter(void* contents, size_t size, size_t nmemb, void* userp);

/* Use like this:
 * QByteArray response;
 * curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
 * curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, QBWriter);
 * finish!
 */
size_t QBWriter(void* contents, size_t size, size_t nmemb, QByteArray* userp);
size_t QSWriter(void* contents, size_t size, size_t nmemb, QString* userp);
// same but with std::string
size_t STDWriter(void* contents, size_t size, size_t nmemb, std::string* userp);

// cry
struct curl_slist;
class CurlKeeper;
class CurlHeader : public NoCopy {
      public:
	~CurlHeader();
	void              add(QString header);
	void              add(QByteArray header);
	void              add(const char* header);
	void              add(std::string_view header);
	void              clear();
	const curl_slist* get() const;
	void              set(CurlKeeper& curl) const;

	[[deprecated("use get")]] const curl_slist* getChunk() const;

      private:
	struct curl_slist* chunk = nullptr;
};

class CurlForm : private NoCopy {
      public:
	CurlForm(CURL* _curl);

	/**
	 * in some case we want to keep track of what is beeing sent
	if you want to skip for something
{
	ResetAfterUse s(mime->saveJson, nullptr);
	mime->add("access_token", campaign->account->kit);
}
*/
	boost::json::object* saveJson = nullptr;

	operator curl_mime*() const;

	curl_mime* get() const;

	void add(const QString& name, const QString& value);
	void add(const QByteArray& name, const QByteArray& value);
	void add(const std::string_view& name, const std::string_view& value);

	void addFile(const std::string& name, const std::string& path);

	void connect() const;
	~CurlForm();

      private:
	void asJson(const std::string_view& name, const std::string_view& value) const;

	// the curl_mimepart do not to be freed as long as they are used
	curl_mime* form = nullptr;
	CURL*      curl = nullptr;
};

class CurlKeeper : private NoCopy {
      public:
	CurlKeeper();
	~CurlKeeper();
	CurlHeader header;

	operator CURL*() const;

	//I have no idea how to find the used url !
	QString url;

	CURL* get() const;

      private:
	CURL* curl = nullptr;
};

// inspired from https://github.com/whoshuu/cpr/blob/master/include/cpr/cprtypes.h
struct CaseInsensitiveCompare {
	bool operator()(QStringView a, QStringView b) const noexcept;
};

class Header : public mapV2<QStringView, QStringView, CaseInsensitiveCompare> {
      public:
	QString serialize() const;
};

[[nodiscard]] Header parseHeader(const QStringView headers);

struct CurlCallResult {
	CurlCallResult();
	QString  errorMsg;
	QString  getError() const;
	CURLcode errorCode = CURLE_OK;
	QString  ip;
	// used to keep alive all the QStringView
	QString headerRaw;
	Header  header;
	QString url;
	// Keep raw as can be binary stuff
	QByteArray result;
	//in case of a redirect
	QByteArray redirectUrl;
	CURLTiming timing;
	long       httpCode = 0;
	bool       ok       = false;
	QString    packDbgMsg(bool extraInfo = false) const;
	bool       fromCache = false;
};

//FIXME do not pass CURL Obj which is an alias to void -.- and of course the type system will not work, use CurlKeeper
/**
 * @brief urlGetContent
 * @param url
 * @param quiet
 * @param curl let use an already bootstrapped curl instance (header / cookie)
 * @return
 */
QByteArray     urlGetContent(const QByteArray& url, bool quiet = false, CURL* curl = nullptr);
QByteArray     urlGetContent(const QString& url, bool quiet = false, CURL* curl = nullptr);
CurlCallResult urlGetContent2(const std::string& url, bool quiet = false, CURL* curl = nullptr);

/**
 * @brief urlGetContent2
 * @param url
 * @param quiet
 * @param curl
 * @param LTS sometime we have a special name for this curl operation (like in clickhouse) so do not manage the local thread status
 * @return
 */
CurlCallResult urlGetContent2(const QByteArray& url, bool quiet = false, CURL* curl = nullptr, bool LTS = true);
CurlCallResult urlGetContent2(const QString& url, bool quiet = false, CURL* curl = nullptr);
CurlCallResult urlGetContent2(const char* url, bool quiet = false, CURL* curl = nullptr);
// TODO rifare la funzione e ritornare un oggetto composito per sapere se è andato a buon fine
CurlCallResult urlPostContent(const QByteArray& url, const QByteArray& post, bool quiet = false, CURL* curl = nullptr);
CurlCallResult urlPostContent(const std::string& url, const std::string& post, bool quiet = false, CURL* curl = nullptr);
//LXC vuole fare il prezzemolino e usa PUT...
CurlCallResult urlPutContent(const QByteViewV2& url, const QByteViewV2& post, CurlKeeper& curl, bool quiet = false);
CurlCallResult urlPutContent(const QByteViewV2& url, const QByteViewV2& post, bool quiet = false);

/**
 * @brief urlGetCached
 * @param url
 * @param fileName
 * @param ttl 0 means DISABLED
 * @param curl
 * @return
 */
template <typename T1, typename T2>
CurlCallResult urlGetCached(const T1& url, const T2 fileName, uint ttl, CURL* curl = nullptr) {
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

enum class Severity {
	none,
	_qDebug,
	_qInfo,
	_qWarning,
	_qCritical
};

// TODO cablare dentro il warn, di modo che non serve mettere quiet true e gestire fuori errori di basso livello, ma hai già dentro tutto nel 98% dei casi
// Buttaci in mezzo tempi risposta e altre cosine carine
class urlGetContentV3 {
	static CurlCallResult get(const QByteArray& url, bool quiet = false, CURL* curl = nullptr);
	Severity              severity = Severity::none;
};

struct QBReaderSt {
	size_t     pos = 0;
	QByteArray data;
};
size_t QBReader(char* ptr, size_t size, size_t nmemb, void* userdata);
