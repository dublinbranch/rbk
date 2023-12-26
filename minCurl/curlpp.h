#ifndef CURLPP_H
#define CURLPP_H

#include <curl/curl.h>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#define NUM_OF_CURL_ERR 1000
//#define DONT_CURL
//#define VERBOSE_ALL

#ifdef DONT_CURL
#warning*********** CURL is not activated!! ***********
#endif //DONT_CURL

struct CurlPPStruct {
	char*  memory = nullptr;
	size_t size   = 0;
	~CurlPPStruct();
};

struct upload_status {
	size_t      bytes_read = 0;
	std::string smtp_payload;
};

/**
 * @brief The CURLpp class
 *
 *
 * libCURL wrapper
 *
 * uses the builder design pattern
 *
 * EXAMPLE:
 *
 *	CURLpp test = CURLpp::Builder()
 *			.set_connect_timeout(2000)
 *			.set_follow_location(1)
 *			.set_url("ciao.php")
 *			.add_http_header("Content-Type: application/json")
 *			.build();						//build() MUST BE THE LAST!!
 *
 *
 * TO ADD A NEW VALUE:
 * 1 - add the new variable in builder with a default value
 * 2 - add the corresponding parameter in CURLpp constructor
 * 3 - add a Builder::set_new_variable with the same style as the given ones (remember return *this; at the end)
 * 4 - add the variable in the return inside Builder::build (the same order as CURLpp ctor)
 * 5 - add the corresponding curl setopt command in the CURLpp ctor
 * 6 - watch out for char pointers, you may want to make a persistent copy as member of CURLpp (e.g. post_fields_)
 *
 */

class CURLpp {
	using vector_of_errors = std::vector<std::map<std::string, std::string>>;

      public:
	class Builder;
	std::string perform();

	~CURLpp();

	std::string getLastError() const;

	static std::vector<std::map<std::string, std::string>> getListOfErrors();

	long getLastHttpCode() const;

	std::string getLastUrl() const;
	std::string getEffectiveUrl() const;
	std::string getLastParam() const;
	std::string getLastResponse() const;
	void        setUrl(const std::string& url);
	void        addHeader(const std::string& header);
	void        resetHeader();
	void        setPost(const std::string& post);
	CURLpp(const CURLpp::Builder& opt);
	CURL* getMarx() const;

	Builder* copia = nullptr; //avoid to go out of scope of certain thing
      private:
	template <typename T>
	void          log(const char* name, T value);
	static size_t writeMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp);

	static size_t smtp_payload_source(void* ptr, size_t size, size_t nmemb, void* userp);
	void          smtp_prepare_message();
	bool          smtp_send(bool rawHtml = false);

	CURL*       marx      = nullptr;
	std::string lastError = "noerr";
	std::string lastUrl;
	std::string lastParam;
	std::string lastResponse;
	char*       effectiveUrl = nullptr;
	long        http_code    = 0;

	static std::mutex       error_mutex;
	static vector_of_errors listOfErrors;
	static int              error_counter;
};

class CURLpp::Builder {

      public:
	int                  connect_timeout = 8000;
	int                  timeout         = 15000;
	int                  post            = 0;
	int                  put             = 0;
	int                  del             = 0;
	int                  patch           = 0;
	int                  get             = 0;
	int                  smtp            = 0;
	int                  follow_location = 0;
	int                  ssl_verifier    = 0;
	int                  use_cookie      = 0;
	int                  use_cookiejar   = 0;
	std::string          useragent;
	std::string          url;
	std::string          cookie;
	std::string          cookiejar;
	curl_slist*          http_header     = nullptr;
	curl_slist*          smtp_recipients = nullptr;
	std::string          smtp_message;
	std::string          smtp_subject;
	std::string          smtp_to;
	std::string          smtp_user;
	std::string          smtp_password;
	std::string          smtp_from;
	std::string          sender_name;
	std::string          post_fields;
	std::string          put_fields;
	std::string          del_fields;
	std::string          patch_fields;
	std::string          get_fields;
	struct upload_status upload_ctx;
#ifdef VERBOSE_ALL
#warning******** CURLpp VERBOSE_ALL ********
	int verbose = 1;
#else
	int verbose = 0;
#endif // VERBOSE_ALL
	int         auth     = 0;
	std::string authCode = "";

	Builder& set_connect_timeout(const int i);
	Builder& set_timeout(const int i);
	Builder& set_post(const int i);
	Builder& set_follow_location(const int i);
	Builder& add_http_header(const char* i);
	Builder& set_verbose(const int i);
	Builder& set_delete_request(const std::string& i);
	Builder& set_auth(const std::string i);
	Builder& set_put_fields(const std::string i);
	Builder& set_post_fields(const std::string i);
	Builder& set_patch_fields(const std::string i);
	Builder& set_get_fields(const std::string i);
	Builder& set_useragent(const std::string i);
	Builder& set_url(const std::string i);
	Builder& set_cookiejar(const std::string i);
	Builder& set_cookie(const std::string i);
	Builder& set_email_details(const std::string msg, const std::string sbj, const std::string to);
	Builder& set_smtp_details(const std::string usr, const std::string pwd, const std::string from);
	Builder& set_ssl_verifier(const int flag);

	CURLpp build();
};

/** In order to launch RTB Won Notification Url the code at rtbhub.h/cpp is adapted to recicle the internal OBJ connection ecc
 *  See: thread_local C_D_Handler Lenin;
 *
 * Is not needed to have "multiple" curl handle for the various host because
 * https://curl.haxx.se/docs/faq.html#What_about_Keep_Alive_or_persist
 * When you use the easy interface the connection cache is kept within the easy handle.
 * If you instead use the multi interface, the connection cache will be kept within the multi handle and
 * will be shared among all the easy handles that are used within the same multi handle.
 *
 */
//GG all forward declaration of template, no one can stop us!
class QByteArray;
template <typename QByteArray>
class QList;

class CurlHandlerWrapper {
      public:
	CurlHandlerWrapper(long timeout_in_milliseconds = 350L);

	~CurlHandlerWrapper();

	void sendNotification(const QList<QByteArray>& UrlList);
	bool sendNotification(const QByteArray& UrlList);

      private:
	CURL* handle{nullptr};
};

#endif // CURLPP_H
