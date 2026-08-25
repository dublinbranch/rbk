#include "authHandler.h"
#include "loginLimiter.h"

#include "rbk/BoostJson/extra.h"
#include "rbk/HTTP/PMFCGI.h"
#include "rbk/HTTP/Payload.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/mustache/extra.h"
#include "rbk/string/stringoso.h"
#include "rbk/string/util.h"

#include <boost/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

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

//same-origin relative path only; empty means fall back to successPath
std::optional<std::string> safeNextPath(std::string_view raw) {
	if (raw.empty()) {
		return {};
	}
	if (raw.find("://") != std::string_view::npos || raw.starts_with("//")) {
		return {};
	}
	if (raw.find("..") != std::string_view::npos || raw.find_first_of("\\\r\n") != std::string_view::npos) {
		return {};
	}
	for (unsigned char ch : raw) {
		if (ch < 0x20 || ch == 0x7F) {
			return {};
		}
	}
	while (raw.starts_with('/')) {
		raw.remove_prefix(1);
	}
	if (raw.empty() || raw.starts_with("//") || raw.find("://") != std::string_view::npos) {
		return {};
	}

	auto pathOnly = raw.substr(0, raw.find_first_of("?#"));
	if (pathOnly == "login" || pathOnly == "logout" || pathOnly == "app/login" || pathOnly == "app/logout"
	    || pathOnly.starts_with("login/") || pathOnly.starts_with("logout/")) {
		return {};
	}
	return std::string(raw);
}

std::string originalTarget(const PMFCGI& status) {
	auto target = status.path;
	if (target.starts_with('/')) {
		target.erase(0, 1);
	}
	return target;
}

std::string requestedNext(const PMFCGI& status) {
	if (auto v = status.post.get("next"); v) {
		return v.val->toStdString();
	}
	if (auto v = status.get.get("next"); v) {
		return v.val->toStdString();
	}
	return {};
}

std::string loginLeafFor(std::string_view nextRaw) {
	if (auto next = safeNextPath(nextRaw); next) {
		const auto pathOnly = std::string_view(*next).substr(0, next->find_first_of("?#"));
		if (pathOnly == "app" || pathOnly.starts_with("app/") || pathOnly.starts_with("api/app/")) {
			return "app/login";
		}
	}
	return "login";
}

std::string loginRedirect(const std::string& basePath, std::optional<uint> error, std::string_view nextRaw,
                          std::string_view leafOverride = {}) {
	const auto leaf = leafOverride.empty() ? loginLeafFor(nextRaw) : std::string(leafOverride);
	std::string loc    = F("{}{}", basePath, leaf);
	bool        first  = true;
	auto        append = [&](std::string_view key, std::string_view value) {
		loc += first ? '?' : '&';
		first = false;
		loc += key;
		loc += '=';
		loc += value;
	};
	if (error) {
		append("error", std::to_string(*error));
	}
	if (auto next = safeNextPath(nextRaw); next) {
		append("next", percentEncoding(*next));
	}
	return loc;
}

std::string successRedirect(const std::string& basePath, std::string_view nextRaw, std::string_view fallback) {
	if (auto next = safeNextPath(nextRaw); next) {
		return F("{}{}", basePath, *next);
	}
	return F("{}{}", basePath, fallback);
}

void setLoginTemplateVars(bj::object& json, const std::string& basePath, std::string_view nextRaw) {
	json["BASE_PATH"] = basePath;
	if (auto next = safeNextPath(nextRaw); next) {
		json["NEXT"] = *next;
	}
	if (auto& decorate = conf().decorateLoginJson; decorate) {
		decorate(json);
	}
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
	auto next     = originalTarget(status);

	const auto tryDevelLogin = [&] {
		return c.develLogin && c.develLogin(status);
	};

	if (auto session = status.cookies->get(c.cookieName); session) {
		switch (c.resumeSession(status, session.val->toStdString())) {
		case SessionState::ok:
			break;
		case SessionState::invalid:
			payload.headers.deleteCookie(c.cookieName.toStdString());
			//stale cookie used to skip develLogin and bounce to login?error=7, which
			//breaks local auto-login after every restart (APCu sessions die with the process)
			if (!tryDevelLogin()) {
				return deny(path, payload, loginRedirect(basePath, 7, next));
			}
			break;
		case SessionState::notLogged:
			if (!tryDevelLogin()) {
				return deny(path, payload, loginRedirect(basePath, {}, next));
			}
			break;
		}
	} else if (!tryDevelLogin()) {
		return deny(path, payload, loginRedirect(basePath, {}, next));
	}

	//project level checks (ACL, account scoping, ...)
	if (c.postLogin && !c.postLogin(status, payload)) {
		return false;
	}

	//preflight check all clear, let's GO!
	return true;
}

UpgradeAuth checkWsUpgrade(PMFCGI& status, int minLevel) {
	auto& c = conf();

	const auto tryDevelLogin = [&] {
		return c.develLogin && c.develLogin(status);
	};

	bool resolved = false;
	if (status.cookies) {
		if (auto session = status.cookies->get(c.cookieName); session) {
			if (!c.resumeSession) {
				return UpgradeAuth::unauthorized;
			}
			switch (c.resumeSession(status, session.val->toStdString())) {
			case SessionState::ok:
				resolved = true;
				break;
			case SessionState::invalid:
			case SessionState::notLogged:
				resolved = tryDevelLogin();
				break;
			}
		} else {
			resolved = tryDevelLogin();
		}
	} else {
		resolved = tryDevelLogin();
	}

	if (!resolved || !c.isLogged || !c.isLogged()) {
		return UpgradeAuth::unauthorized;
	}
	if (c.hasLevel && !c.hasLevel(minLevel)) {
		return UpgradeAuth::forbidden;
	}
	return UpgradeAuth::ok;
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
	auto next     = requestedNext(status);
	const auto path = status.url.url.path().mid(1);
	const bool appLogin = path == "app/login";
	const auto& tmpl = (appLogin && !c.appLoginTemplate.empty()) ? c.appLoginTemplate : c.loginTemplate;
	const auto fallback = appLogin
	                          ? (c.appSuccessPath.empty() ? "app/" : c.appSuccessPath)
	                          : c.successPath;

	if (auto email = status.post.get("email"); email) {
		//if email is set we assume you want to login
		const auto trimmedEmail = email.val->trimmed();
		auto       password     = status.post.rq("password").trimmed();

		LoginLimiter::Guard inflight;
		if (c.loginLimiter) {
			auto slot = LoginLimiter::instance().tryBegin(status.remoteIp, trimmedEmail.toStdString());
			if (slot.admit != LoginLimiter::Admit::ok) {
				if (c.onLoginRateLimited) {
					c.onLoginRateLimited(status, trimmedEmail, LoginLimiter::admitReason(slot.admit));
				}
				const auto retry   = slot.retryAfterSec ? slot.retryAfterSec : 10u;
				payload.statusCode = 429;
				payload.headers.insert({"Retry-After", std::to_string(retry)});
				bj::object json;
				setLoginTemplateVars(json, basePath, next);
				json["ERROR_MESSAGE"] = "Too many login attempts, try again shortly.";
				payload.html          = mustache(tmpl, json);
				return;
			}
			inflight = std::move(slot.guard);
		}

		auto outcome = c.login(status, trimmedEmail, password);
		if (c.loginLimiter && outcome.result != LoginResult::rateLimited) {
			if (outcome.result == LoginResult::ok) {
				LoginLimiter::instance().recordSuccess(trimmedEmail.toStdString());
			} else {
				LoginLimiter::instance().recordFailure(status.remoteIp, trimmedEmail.toStdString());
			}
		}
		switch (outcome.result) {
		case LoginResult::ok:
			payload.setCookie(c.cookieName.toStdString(), outcome.sessionId, c.cookieTTL, true, c.cookieSecure);
			payload.redirect(successRedirect(basePath, next, fallback));
			break;

		case LoginResult::invalidEmail:
			payload.redirect(loginRedirect(basePath, 1, next, appLogin ? "app/login" : "login"));
			break;

		case LoginResult::invalidPassword:
			payload.redirect(loginRedirect(basePath, 2, next, appLogin ? "app/login" : "login"));
			break;

		case LoginResult::rateLimited: {
			const auto retry = outcome.retryAfterSec ? outcome.retryAfterSec : 10u;
			payload.statusCode = 429;
			payload.headers.insert({"Retry-After", std::to_string(retry)});
			bj::object json;
			setLoginTemplateVars(json, basePath, next);
			json["ERROR_MESSAGE"] = "Too many login attempts, try again shortly.";
			payload.html          = mustache(tmpl, json);
			break;
		}

		default:
			payload.redirect(loginRedirect(basePath, 3, next, appLogin ? "app/login" : "login"));
			break;
		}
		return;
	}

	bj::object json;
	setLoginTemplateVars(json, basePath, next);

	if (auto error = status.get.get<uint>("error"); error) {
		json["ERROR_MESSAGE"] = c.loginErrorMessage ? c.loginErrorMessage(error.val) : defaultLoginErrorMessage(error.val);
	}

	payload.html = mustache(tmpl, json);
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
	const auto path = status.url.url.path().mid(1);
	const auto leaf = (path == "app/logout") ? "app/login" : "login";
	payload.redirect(F("{}{}", status.getBasePath(), leaf));
}

} // namespace rbk::Auth
