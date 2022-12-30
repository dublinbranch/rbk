#include "Payload.h"
#include <fmt/format.h>

void Headers::setCookie(const std::string& name, const std::string& value, uint ttl, bool sameSite) {
	//	string secureS = secure ? "secure" : "";
	//	string httpOnlyS = httponly ? "HttpOnly" : "";
	// path={:path}; domain={:domain};
	//https://developers.google.com/search/blog/2020/01/get-ready-for-new-samesitenone-secure?hl=it
	// secure;  is not set in local development, and only block that is sent on port 80 looks like
	auto SameSite = sameSite ? "SameSite=Lax" : "SameSite=None";
	auto row      = fmt::format(R"({}={}; Max-Age={}; {}; secure; HttpOnly)", name, value, ttl, SameSite);
	insert({"Set-Cookie", row});
}
