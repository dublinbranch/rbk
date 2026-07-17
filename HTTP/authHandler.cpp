#include "authHandler.h"

#include "rbk/BoostJson/extra.h"
#include "rbk/HTTP/PMFCGI.h"
#include "rbk/HTTP/Payload.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/mustache/extra.h"

#include <boost/json.hpp>

namespace bj = boost::json;

namespace rbk::Auth {

namespace {

bool isApiCall(const Conf& c, const QString& path) {
	if (c.isApiCall) {
		return c.isApiCall(path);
	}
	return path.startsWith("api/");
}

//short circuit the request, api calls get a 401 json, humans get the login page
bool deny(const QString& path, Payload& payload, const std::string& location) {
	if (isApiCall(conf(), path)) {
		writeJsonError(payload, 401, "login required");
	} else {
		payload.setStandardHeaders();
		payload.redirect(location);
	}
	return false;
}

std::string defaultLoginErrorMessage(uint error) {
	switch (error) {
	case 1:
		return "Invalid email or username";
	case 2:
		return "Invalid password";
	case 7:
		return "Invalid session";
	default:
		return "Login Error";
	}
}

} // namespace

Conf& conf() {
	static Conf c;
	return c;
}

void writeJsonError(Payload& payload, int statusCode, const std::string& message) {
	bj::object json;
	json["status"]     = "error";
	json["message"]    = message;
	payload.statusCode = statusCode;
	payload.html       = pretty_print(json);
}

void denyRoute(PMFCGI& status, Payload& payload, int statusCode) {
	if (statusCode == 401) {
		writeJsonError(payload, 401, "login required");
		return;
	}
	if (auto& c = conf(); c.auditBlockedRoute) {
		c.auditBlockedRoute(status);
	}
	writeJsonError(payload, statusCode, "insufficient privileges");
}

bool loginManager(PMFCGI& status, Payload& payload) {
	auto& c    = conf();
	auto  path = status.url.url.path().mid(1);

	if (c.publicPaths.contains(path)) {
		return true;
	}
	//device traffic and static assets, the websockets are upgraded before we are even called
	for (auto& prefix : c.publicPrefixes) {
		if (path.startsWith(prefix)) {
			return true;
		}
	}

	auto basePath = status.getBasePath();

	if (auto session = status.cookies->get(c.cookieName); session) {
		switch (c.resumeSession(status, session.val->toStdString())) {
		case SessionState::ok:
			break;
		case SessionState::invalid:
			payload.headers.deleteCookie(c.cookieName.toStdString());
			return deny(path, payload, F("{}login?error=7", basePath));
		case SessionState::notLogged:
			return deny(path, payload, F("{}login", basePath));
		}
	} else if (c.develLogin && c.develLogin(status)) {
		//development auto login, nothing else to do
	} else {
		return deny(path, payload, F("{}login", basePath));
	}

	//project level checks (ACL, account scoping, ...)
	if (c.postLogin && !c.postLogin(status, payload)) {
		return false;
	}

	//preflight check all clear, let's GO!
	return true;
}

void loginPage(PMFCGI& status, Payload& payload) {
	auto& c = conf();
	status.decodePost();

	if (c.prepareAnonymous) {
		c.prepareAnonymous(status);
	}

	payload.setStandardHeaders();
	payload.statusCode = 200;

	auto basePath = status.getBasePath();

	if (auto email = status.post.get("email"); email) {
		//if email is set we assume you want to login
		auto password = status.post.rq("password").trimmed();

		auto outcome = c.login(status, email.val->trimmed(), password);
		switch (outcome.result) {
		case LoginResult::ok:
			payload.setCookie(c.cookieName.toStdString(), outcome.sessionId, c.cookieTTL, true, c.cookieSecure);
			payload.redirect(F("{}{}", basePath, c.successPath));
			break;

		case LoginResult::invalidEmail:
			payload.redirect(F("{}login?error=1", basePath));
			break;

		case LoginResult::invalidPassword:
			payload.redirect(F("{}login?error=2", basePath));
			break;

		default:
			payload.redirect(F("{}login?error=3", basePath));
			break;
		}
		return;
	}

	bj::object json;
	json["BASE_PATH"] = basePath;

	if (auto error = status.get.get<uint>("error"); error) {
		json["ERROR_MESSAGE"] = c.loginErrorMessage ? c.loginErrorMessage(error.val) : defaultLoginErrorMessage(error.val);
	}

	payload.html = mustache(c.loginTemplate, json);
}

void logout(PMFCGI& status, Payload& payload) {
	auto& c = conf();
	payload.setStandardHeaders();

	if (auto session = status.cookies->get(c.cookieName); session) {
		if (c.logout) {
			c.logout(status, session.val->toStdString());
		}
	}
	payload.headers.deleteCookie(c.cookieName.toStdString());
	payload.redirect(F("{}login", status.getBasePath()));
}

} // namespace rbk::Auth
