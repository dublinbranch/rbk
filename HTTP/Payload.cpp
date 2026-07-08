#include "Payload.h"
#include <fmt/format.h>

//FIXME add the path parameter
void Headers::setCookie(std::string_view name, std::string_view value, uint ttl, bool sameSite, bool secure) {
	//NB: the Secure attribute makes browsers drop the cookie over plain HTTP, so it is
	//only appropriate behind TLS; the caller decides via the `secure` argument.
	std::string secureS = secure ? " Secure;" : "";
	//	string httpOnlyS = httponly ? "HttpOnly" : "";
	// path={:path}; domain={:domain};
	//https://developers.google.com/search/blog/2020/01/get-ready-for-new-samesitenone-secure?hl=it
	auto        SameSite = sameSite ? "SameSite=Strict" : "SameSite=Lax";
	std::string row      = fmt::format(R"({}={}; Max-Age={}; path=/ ; {};{} HttpOnly)", name, value, ttl, SameSite, secureS);
	insert({"Set-Cookie", row});
}

void Headers::deleteCookie(const StringAdt& name) {
	auto row = fmt::format(R"({}=; Max-Age={}; path=/ ; {}; HttpOnly)", name, 0, "SameSite=Lax");
	insert({"Set-Cookie", row});
}
