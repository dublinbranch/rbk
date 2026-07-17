#ifndef RBK_HTTP_AUTHHANDLER_H
#define RBK_HTTP_AUTHHANDLER_H

#include <QString>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "rbk/mapExtensor/mapV2.h"

class PMFCGI;
class Payload;

/**
 * Generic session / login engine, the project keeps full ownership of its user class:
 * every user related operation goes through the callbacks in Conf, the engine never
 * stores a user pointer (store it wherever you like, thread local, request context, ...).
 *
 * Wiring (see digitalSpine http/auth_handler.cpp for a complete example):
 *   fill rbk::Auth::conf() at startup, then
 *   beastConf.loginManager = rbk::Auth::loginManager;
 *   routing: {"login", rbk::Auth::loginPage}, {"logout", rbk::Auth::logout}
 *   guarded: {"api/xyz", rbk::Auth::guard<myLevel, myHandler>}
 */
namespace rbk::Auth {

enum class LoginResult {
	ok,
	invalidEmail,
	invalidPassword,
	error
};

enum class SessionState {
	ok,        //valid session, user is logged
	invalid,   //unknown session id, the cookie will be deleted
	notLogged  //session resolved but the user is not (or no longer) logged
};

struct LoginOutcome {
	LoginResult result = LoginResult::error;
	//required when result == ok, becomes the session cookie value
	std::string sessionId;
};

struct Conf {
	//paths served WITHOUT a logged in user (compared against the url path with the leading / stripped)
	mapV2<QString, bool> publicPaths;    //exact match
	std::vector<QString> publicPrefixes; //startsWith match (static assets, device traffic, ...)

	QString cookieName   = "session";
	uint    cookieTTL    = 3600 * 24;
	bool    cookieSecure = true;

	//mustache template of the login form, receives BASE_PATH and ERROR_MESSAGE
	std::filesystem::path loginTemplate;
	//where to land after a successful login, appended to the base path
	std::string successPath = "index";

	//non logged api calls get a json 401 instead of a redirect to the login page, default path.startsWith("api/")
	std::function<bool(const QString& path)> isApiCall;
	//override the default error=N -> message mapping of the login page
	std::function<std::string(uint error)> loginErrorMessage;

	/* ---- project hooks ---- */

	//resolve the session cookie and store the user on your side (also set there IP / UA if you track them)
	std::function<SessionState(PMFCGI& status, const std::string& sessionId)> resumeSession;

	//optional: auto login for development, return true if a user was logged in
	std::function<bool(PMFCGI& status)> develLogin;

	//optional: runs after a successful preflight (ACL, account scoping, ...)
	//return false to short circuit the request, in that case write the response yourself
	std::function<bool(PMFCGI& status, Payload& payload)> postLogin;

	//optional: called at the start of loginPage, typically to set up an anonymous user for logging
	std::function<void(PMFCGI& status)> prepareAnonymous;

	//check the credentials, on ok also register the session and return its id
	std::function<LoginOutcome(PMFCGI& status, const QString& email, const QString& password)> login;

	//invalidate the session (the engine deletes the cookie and redirects to the login page)
	std::function<void(PMFCGI& status, const std::string& sessionId)> logout;

	/* ---- used by guard / denyRoute ---- */
	std::function<bool()>         isLogged;
	std::function<bool(int level)> hasLevel;
	//optional: audit a route blocked with 403
	std::function<void(PMFCGI& status)> auditBlockedRoute;
};

//the singleton filled by the project at startup, before the server starts
Conf& conf();

//session preflight, registered as BeastConf::loginManager so it runs before any routing
bool loginManager(PMFCGI& status, Payload& payload);

//GET renders the login form, POST (email + password) performs the login and sets the session cookie
void loginPage(PMFCGI& status, Payload& payload);
void logout(PMFCGI& status, Payload& payload);

//short circuit a routed handler: 401 not logged, 403 insufficient level (with audit trail hook)
void denyRoute(PMFCGI& status, Payload& payload, int statusCode);

void writeJsonError(Payload& payload, int statusCode, const std::string& message);

/**
 * Wraps a routed handler with a minimum required level, so the privilege needed
 * is stated once, directly in the routing table. minLevel is an int so any project
 * enum can be used, the comparison is delegated to Conf::hasLevel.
 * (routingSimple stores plain function pointers, hence a template and not a lambda)
 */
template <int minLevel, void (*handler)(PMFCGI& status, Payload& payload)>
void guard(PMFCGI& status, Payload& payload) {
	auto& c = conf();
	//the loginManager preflight already resolved the user for every non whitelisted path
	if (!c.isLogged || !c.isLogged()) {
		denyRoute(status, payload, 401);
		return;
	}
	if (!c.hasLevel(minLevel)) {
		denyRoute(status, payload, 403);
		return;
	}
	handler(status, payload);
}

} // namespace rbk::Auth

#endif // RBK_HTTP_AUTHHANDLER_H
