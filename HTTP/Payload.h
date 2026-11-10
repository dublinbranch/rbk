#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "rbk/number/intTypes.h"
#include "rbk/string/stringoso.h"
#include <functional>
#include <map>
#include <optional>
#include <string>

class Headers : public std::multimap<std::string, std::string> {
      public:
	/**
	 * @brief setCookie https://www.php.net/manual/en/function.setcookie.php
	 * @param name
	 * @param value
	 * @param ttl, after how much time will expire
	 * #param sameSite, we are a backend, is normal we are called from other site
	 *
	 * @param path The path on the server in which the cookie will be available on. If set to '/', the cookie will be available within the entire domain.
	        If set to '/foo/', the cookie will only be available within the /foo/ directory and all sub-directories such as /foo/bar/ of domain.
	 * @param domain
	        The (sub)domain that the cookie is available to.
	        Setting this to a subdomain (such as 'www.example.com') will make the cookie available to that subdomain and all other sub-domains of it (i.e. w2.www.example.com).
	        To make the cookie available to the whole domain (including all subdomains of it), simply set the value to the domain name ('example.com', in this case).
	 * @param secure
	 * @param httponly can't really see a valid reason to have default OFF
	 *
	 *
	 * Standard beast way is in https://github.com/boostorg/beast/issues/1425
	 * res.set(field::set_cookie, "mycookie=123");
	 * -------
	 * for(auto param : http::param_list(req[field::cookie]))
	        std::cout << "Cookie '" << param.first << "' has value '" << param.second << "'\n";
	 */
	//Not implemented const std::string& path, const std::string& domain, bool secure, bool httponly
	void setCookie(std::string_view name,
	               std::string_view value,
	               uint             ttl,
	               bool             sameSite = false,
	               bool             secure   = true);
	void deleteCookie(const StringAdt& name);
};

class PMFCGI;

/* A file answer (a large download): rbk sends the file from disk part by part, not from memory.
 * Set statusCode, mime and headers as for html; html is not used. rbk opens the file after immediate(), and
 * Content-Length is the size of the file at that time. The file stays open until the send ends, so the caller
 * can remove it in onDone.
 * The time limit of the connection (30 s) applies to each part: the send stops only when the client takes
 * nothing for 30 s. Behind nginx also set the header "X-Accel-Buffering: no", or nginx keeps the file in its
 * own buffer first.
 */
struct PayloadFile {
	std::string path;
	/* Runs once on the HTTP worker thread when the send ends, before deferred().
	 * complete = true: all the bytes of the file are written to the socket.
	 * complete = false: the file can not be opened (the client gets a 500), the client went away or took nothing
	 * for the time limit, a read of the file failed, or immediate() threw after it set the file. */
	std::function<void(bool complete)> onDone;
};

class Payload {
      public:
	bool     alreadySent = false;
	unsigned statusCode  = 200;
	//a response can not exists without a request no ?
	PMFCGI*     status = nullptr;
	std::string html;
	std::string mime = "text/html; charset=utf-8";
	Headers     headers;
	// Set: a file answer instead of html, see PayloadFile.
	std::optional<PayloadFile> file;
	void        setStandardHeaders(bool addCors = true);
	void        setCacheHeader(uint ttl);

	//some quality of life functions
	void redirect(const StringAdt& location);
	void setCookie(const StringAdt& key, const StringAdt& value, u32 cookieTTL, bool sameSite = true, bool secure = true);
};

#endif // PAYLOAD_H
