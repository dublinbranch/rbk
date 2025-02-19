#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "rbk/misc/intTypes.h"
#include "rbk/string/stringoso.h"
#include <map>
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
	void deleteCookie(std::string_view name);
};

class PMFCGI;

class Payload {
      public:
	bool     alreadySent = false;
	unsigned statusCode  = 200;
	//a response can not exists without a request no ?
	PMFCGI*     status = nullptr;
	std::string html;
	std::string mime = "text/html";
	Headers     headers;
	void        setStandardHeaders(bool addCors = true);
	void        setCacheHeader(uint ttl);

	//some quality of life functions
	void redirect(const StringAdt& location);
	void setCookie(const std::string_view& key, const StringAdt& value, u32 cookieTTL, bool sameSite = true, bool secure = true);
};

#endif // PAYLOAD_H
