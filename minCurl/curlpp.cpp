#include "curlpp.h"
#include "iostream"
#include <QByteArray>
#include <QList>
#include <cstring>
#include <ctime>
using namespace std;

std::vector<std::map<std::string, std::string>> CURLpp::listOfErrors(NUM_OF_CURL_ERR);
int                                             CURLpp::error_counter = 0;
std::mutex                                      CURLpp::error_mutex;

/**
 * @brief CURLpp::perform
 *
 * performs the curl code with the parameters given from the builder
 * @return a string containing "error:" if something went wrong, the curl response otherwise
 */
std::string CURLpp::perform() {
	lastError.clear();
	CURLcode     res;
	std::string  response;
	CurlPPStruct chunk;
	chunk.memory = (char*)malloc(1); // will be grown as needed by the realloc
	chunk.size   = 0;                // no data at this point

	curl_easy_setopt(marx, CURLOPT_WRITEFUNCTION, CURLpp::writeMemoryCallback);
	curl_easy_setopt(marx, CURLOPT_WRITEDATA, (void*)&chunk);

#ifdef DONT_CURL
	response = "{\"response\":{\"id\":42},\"id\":43,\"campaign\":{\"id\":44},\"success\":1667}";
	log("response", response);
#else
	res = curl_easy_perform(marx);

	if (res == CURLE_OK) {
		curl_easy_getinfo(marx, CURLINFO_RESPONSE_CODE, &http_code);
		curl_easy_getinfo(marx, CURLINFO_EFFECTIVE_URL, &effectiveUrl);
		if (http_code == 200 || http_code == 250) { //250 is ok for mail
			if (chunk.size > 0) {
				response.append(chunk.memory, chunk.size);
				log("response", chunk.memory);
			}
		} else {
			if (chunk.size > 0) {
				response.append(chunk.memory, chunk.size);
				log("httpcode", http_code);
				log("response", chunk.memory);
				lastError = "error: http code: " + std::to_string(http_code) + ", " + response + ", url:" + copia->url;
			} else {
				lastError = "error: http code: " + std::to_string(http_code) + " error message: " + curl_easy_strerror(res) + ", no response payload, url:" + copia->url;
			}
		}
	} else {
		log("curl errorcode", res);
		log("curl error", curl_easy_strerror(res));
		lastError = "error: curl error: "s + curl_easy_strerror(res) + ", url:" + copia->url + ", timeout:" + std::to_string(copia->timeout);
	}
#endif

	if (!lastError.empty()) {
		std::lock_guard<std::mutex>        el_gringo(error_mutex);
		std::map<std::string, std::string> error;
		std::time_t                        c_ts = std::time(nullptr);
		error.insert(std::pair<std::string, std::string>("time", std::asctime(std::localtime(&c_ts))));
		error.insert(std::pair<std::string, std::string>("error", lastError));
		listOfErrors[error_counter++] = error;
		error_counter                 = error_counter % NUM_OF_CURL_ERR;
	}

	lastResponse = response;

	return response;
}

CURLpp::~CURLpp() {
	// clean CURL handler
	curl_easy_cleanup(marx);

	// free the custom headers
	curl_slist_free_all(copia->http_header);
	copia->http_header = nullptr;
	curl_slist_free_all(copia->smtp_recipients);
	copia->smtp_recipients = nullptr;

	delete copia;
}

size_t CURLpp::writeMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	size_t realsize = size * nmemb;
	auto   mem      = (CurlPPStruct*)userp;

	mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == nullptr) {
		/* out of memory! */
		printf("not enough memory (realloc returned nullptr)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

size_t CURLpp::smtp_payload_source(void* ptr, size_t size, size_t nmemb, void* userp) {
	struct upload_status* upload_ctx = (struct upload_status*)userp;

	if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
		return 0;
	}
	auto upload_size = upload_ctx->smtp_payload.size();
	auto to_read     = size * nmemb;
	to_read          = min(to_read, upload_size - upload_ctx->bytes_read);

	auto begin = upload_ctx->smtp_payload.c_str() + upload_ctx->bytes_read;

	memcpy(ptr, begin, to_read);

	upload_ctx->bytes_read += to_read;

	return to_read;
}

void CURLpp::smtp_prepare_message() {
	auto& smtp_payload = copia->upload_ctx.smtp_payload;
	smtp_payload.clear();
	smtp_payload.append("To: <" + copia->smtp_to + "> \r\n");
	smtp_payload.append("From:" + copia->sender_name + "<" + copia->smtp_from + "> \r\n");
	//	smtp_payload.push_back("Cc:   \r\n");
	smtp_payload.append("Subject: " + copia->smtp_subject + "\r\n");
	smtp_payload.append("MIME-Version: 1.0\r\n");
	smtp_payload.append("Content-Type: text/html; charset=UTF-8\r\n");
	smtp_payload.append("\r\n");
	smtp_payload.append(copia->smtp_message + "\r\n");
}

bool CURLpp::smtp_send(bool rawHtml) {
	if (!rawHtml) {
		smtp_prepare_message();
	}

	copia->upload_ctx.bytes_read = 0;

	log("smtp_to", copia->smtp_to);
	log("smtp_message", copia->smtp_message);
	log("smtp_subject", copia->smtp_subject);

	lastParam = "to:" + copia->smtp_to +
	            "msg: " + copia->smtp_message +
	            "sbj: " + copia->smtp_subject;

	curl_easy_setopt(marx, CURLOPT_USERNAME, copia->smtp_user.c_str());
	curl_easy_setopt(marx, CURLOPT_PASSWORD, copia->smtp_password.c_str());

	// security
	//curl_easy_setopt(marx, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

	curl_easy_setopt(marx, CURLOPT_MAIL_FROM, copia->smtp_from.c_str());

	copia->smtp_recipients = curl_slist_append(copia->smtp_recipients, copia->smtp_to.c_str());
	curl_easy_setopt(marx, CURLOPT_MAIL_RCPT, copia->smtp_recipients);

	/* We're using a callback function to specify the payload (the headers and
	 * body of the message). You could just use the CURLOPT_READDATA option to
	 * specify a FILE pointer to read from. */

	curl_easy_setopt(marx, CURLOPT_READFUNCTION, smtp_payload_source);
	curl_easy_setopt(marx, CURLOPT_READDATA, &copia->upload_ctx);
	curl_easy_setopt(marx, CURLOPT_UPLOAD, 1L);
	return true;
}

CURL* CURLpp::getMarx() const {
	return marx;
}

std::string CURLpp::getLastResponse() const {
	return lastResponse;
}

void CURLpp::setUrl(const string& url) {
	curl_easy_setopt(marx, CURLOPT_URL, url.c_str());
}

void CURLpp::addHeader(const string& header) {
	copia->http_header = curl_slist_append(copia->http_header, header.c_str());
	curl_easy_setopt(marx, CURLOPT_HTTPHEADER, copia->http_header);
}

void CURLpp::resetHeader() {
	curl_slist_free_all(copia->http_header);
	copia->http_header = nullptr;
}

void CURLpp::setPost(const std::string& post) {
	curl_easy_setopt(marx, CURLOPT_POST, 1);
	curl_easy_setopt(marx, CURLOPT_POSTFIELDS, post.c_str());
	log("post_fields", post);
	lastParam = post;
}

std::string CURLpp::getLastParam() const {
	return lastParam;
}

std::string CURLpp::getLastUrl() const {
	return lastUrl;
}

string CURLpp::getEffectiveUrl() const {
	if (effectiveUrl == nullptr) {
		return "";
	}
	return effectiveUrl;
}

long CURLpp::getLastHttpCode() const {
	return http_code;
}

/**
 * @brief CURLpp::getListOfErrors
 *
 * returns the last 50 errors ordered by time descending
 * @return
 */
std::vector<std::map<std::string, std::string>> CURLpp::getListOfErrors() {
	std::lock_guard<std::mutex> el_gringo(error_mutex);
	if (error_counter == 0) {
		vector_of_errors first_part(listOfErrors.begin(), listOfErrors.end());
		reverse(first_part.begin(), first_part.end());
		return first_part;

	} else {
		vector_of_errors first_part(listOfErrors.begin() + error_counter, listOfErrors.end());
		vector_of_errors second_part(listOfErrors.begin(), listOfErrors.begin() + error_counter - 1);

		first_part.insert(first_part.end(), second_part.begin(), second_part.end());
		reverse(first_part.begin(), first_part.end());
		return first_part;
	}
}

CURLpp::CURLpp(const Builder& opt)
    : marx(curl_easy_init()) {
	copia = new Builder(opt);
	curl_easy_setopt(marx, CURLOPT_HTTPHEADER, copia->http_header);
	curl_easy_setopt(marx, CURLOPT_CONNECTTIMEOUT_MS, copia->connect_timeout);
	curl_easy_setopt(marx, CURLOPT_TIMEOUT_MS, copia->timeout);
	curl_easy_setopt(marx, CURLOPT_URL, copia->url.c_str());
	curl_easy_setopt(marx, CURLOPT_USERAGENT, copia->useragent.c_str());
	curl_easy_setopt(marx, CURLOPT_FOLLOWLOCATION, copia->follow_location);
	curl_easy_setopt(marx, CURLOPT_VERBOSE, copia->verbose);
	curl_easy_setopt(marx, CURLOPT_SSL_VERIFYPEER, copia->ssl_verifier);
	curl_easy_setopt(marx, CURLOPT_SSL_VERIFYHOST, copia->ssl_verifier);
	lastUrl = copia->url;

	if (copia->use_cookie > 0) {
		curl_easy_setopt(marx, CURLOPT_COOKIEFILE, copia->cookie.c_str());
	}
	if (copia->use_cookiejar > 0) {
		curl_easy_setopt(marx, CURLOPT_COOKIEJAR, copia->cookiejar.c_str());
	}

	if (copia->post > 0) {
		curl_easy_setopt(marx, CURLOPT_POST, copia->post);
		curl_easy_setopt(marx, CURLOPT_POSTFIELDS, copia->post_fields.c_str());
		log("post_fields", copia->post_fields);
		lastParam = copia->post_fields;
	} else if (copia->del) {
		curl_easy_setopt(marx, CURLOPT_CUSTOMREQUEST, "DELETE");
		curl_easy_setopt(marx, CURLOPT_POSTFIELDS, copia->del_fields.c_str());
		log("DELETE REQUEST", copia->del_fields);
		lastParam = copia->del_fields;
	} else if (copia->put > 0) {
		curl_easy_setopt(marx, CURLOPT_CUSTOMREQUEST, "PUT");
		curl_easy_setopt(marx, CURLOPT_POSTFIELDS, copia->put_fields.c_str());
		log("put_fields", copia->put_fields);
		lastParam = copia->put_fields;
	} else if (copia->patch > 0) {
		curl_easy_setopt(marx, CURLOPT_CUSTOMREQUEST, "PATCH");
		curl_easy_setopt(marx, CURLOPT_POSTFIELDS, copia->patch_fields.c_str());
		log("patch_fields", copia->patch_fields);
		lastParam = copia->patch_fields;
	} else if (copia->get > 0) {
		curl_easy_setopt(marx, CURLOPT_CUSTOMREQUEST, "GET");
		curl_easy_setopt(marx, CURLOPT_POSTFIELDS, copia->get_fields.c_str());
		log("get_fields", copia->get_fields);
		lastParam = copia->get_fields;
	} else if (copia->smtp > 0) {
		smtp_send();
	} else {
		log("no protocol specified", "I guess you want to go for GET");
	}

	if (copia->auth) {
		curl_easy_setopt(marx, CURLOPT_USERPWD, copia->authCode.c_str());
	}
	log("url", copia->url);
}

std::string CURLpp::getLastError() const {
	return lastError;
}

template <typename T>
void CURLpp::log(const char* name, T value) {
	if (copia->verbose) {
		std::cout << "\x1B[32m" << name << ":\x1B[0m " << value << "\n";
	}
}

CURLpp::Builder& CURLpp::Builder::set_connect_timeout(const int i) {
	this->connect_timeout = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_timeout(const int i) {
	this->timeout = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_post(const int i) {
	this->post = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_follow_location(const int i) {
	this->follow_location = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::add_http_header(const char* i) {
	http_header = curl_slist_append(http_header, i);
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_verbose(const int i) {
	this->verbose = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_delete_request(const string& i) {
	this->del        = 1;
	this->del_fields = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_auth(const string i) {
	this->auth     = 1;
	this->authCode = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_put_fields(const string i) {
	this->put  = 1;
	put_fields = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_post_fields(const string i) {
	this->post        = 1;
	this->post_fields = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_patch_fields(const string i) {
	this->patch        = 1;
	this->patch_fields = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_get_fields(const string i) {
	this->get        = 1;
	this->get_fields = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_useragent(const string i) {
	this->useragent = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_url(const string i) {
	this->url = i;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_cookiejar(const string i) {
	this->cookiejar     = i;
	this->use_cookiejar = 1;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_cookie(const string i) {
	this->cookie     = i;
	this->use_cookie = 1;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_email_details(const string msg, const string sbj, const string to) {
	this->smtp_message = msg;
	this->smtp_subject = sbj;
	this->smtp_to      = to;
	this->smtp         = 1;
	this->url          = "smtp://seisho.us:25";
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_smtp_details(string_view usr, string_view pwd, string_view from, string_view url_) {
	this->smtp_from     = from;
	this->smtp_user     = usr;
	this->smtp_password = pwd;
	this->smtp          = 1;
	this->url           = url_;
	return *this;
}

CURLpp::Builder& CURLpp::Builder::set_ssl_verifier(const int flag) {
	this->ssl_verifier = flag;
	return *this;
}

CURLpp CURLpp::Builder::build() {
	return CURLpp(*this);
}

CurlHandlerWrapper::CurlHandlerWrapper(long timeout_in_milliseconds) {
	handle = curl_easy_init();
	if (handle) {
		curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, timeout_in_milliseconds);
		//disabel any debug output
		curl_easy_setopt(handle, CURLOPT_VERBOSE, false);
		//disable spamming Context Switch
		curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1);
		//disable writing to stdout
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)nullptr);
	}
}

CurlHandlerWrapper::~CurlHandlerWrapper() {
	curl_easy_cleanup(handle);
}

void CurlHandlerWrapper::sendNotification(const QList<QByteArray>& UrlList) {
	/* Allocate one CURL handle per transfer */
	for (const auto& e : UrlList) {
		sendNotification(e);
	}
}

bool CurlHandlerWrapper::sendNotification(const QByteArray& UrlList) {
	if (not handle) {
		return false;
	} // a valid handle is needed
	bool result = true;

	curl_easy_setopt(handle, CURLOPT_URL, UrlList.constData());

	bool ok = (CURLE_OK == curl_easy_perform(handle));
	///@todo log in a better way, this is only spamming
	//		if( not ok ) {
	//			std::cout << "Failed to send Notification of banner_id " << banner_id << " to " << e.toStdString();
	//		}
	result &= ok; // mark if error but keep on going

	return result;
}

CurlPPStruct::~CurlPPStruct() {
	size = 0;
	if (memory) {
		free(memory);
		memory = nullptr;
	}
}
