#ifndef RBK_HTTP_AUTHHANDLER_H
#define RBK_HTTP_AUTHHANDLER_H

#include <QString>
#include <boost/json/object.hpp>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
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
	error,
	rateLimited
};

enum class SessionState {
	ok,        //valid session, user is logged
	invalid,   //unknown session id, the cookie will be deleted
	notLogged  //session resolved but the user is not (or no longer) logged
};

// Websocket upgrades skip loginManager. Same cookie / develLogin / hasLevel
// path, for the browser endpoints that must not stay anonymous.
enum class UpgradeAuth {
	ok,
	unauthorized,
	forbidden
};

struct LoginOutcome {
	LoginResult result = LoginResult::error;
	//required when result == ok, becomes the session cookie value
	std::string sessionId;
	//used when result == rateLimited; 0 means the engine picks a default
	unsigned retryAfterSec = 0;
};

struct Conf {
	//paths served WITHOUT a logged in user (compared against the url path with the leading / stripped)
	mapV2<QString, bool> publicPaths;    //exact match
	std::vector<QString> publicPrefixes; //startsWith match (static assets, device traffic, ...)

	QString cookieName   = "session";
	uint    cookieTTL    = 3600 * 24;
	bool    cookieSecure = true;

	//mustache template of the login form, receives BASE_PATH, ERROR_MESSAGE, and NEXT
	std::filesystem::path loginTemplate;
	//mobile /app/login template; empty falls back to loginTemplate
	std::filesystem::path appLoginTemplate;
	//fallback after a successful login when next is missing or unsafe, appended to the base path
	std::string successPath = "index";
	//fallback after a successful /app/login when next is missing or unsafe
	std::string appSuccessPath = "app/";

	//non logged api calls get a json 401 instead of a redirect to the login page, default path.startsWith("api/")
	std::function<bool(const QString& path)> isApiCall;
	//override the default error=N -> message mapping of the login page
	std::function<std::string(uint error)> loginErrorMessage;

	/* ---- project hooks ---- */

	//resolve the session cookie and store the user on your side (also set there IP / UA if you track them)
	std::function<SessionState(PMFCGI& status, const std::string& sessionId)> resumeSession;

	//optional: auto login for development, return the session id of the logged user (empty = no
	//devel user). The engine sets the cookie, so the next requests resume the session and do not
	//log in again
	std::function<std::string(PMFCGI& status)> develLogin;

	//optional: runs after a successful preflight (ACL, account scoping, ...)
	//return false to short circuit the request, in that case write the response yourself
	std::function<bool(PMFCGI& status, Payload& payload)> postLogin;

	//optional: called at the start of loginPage, typically to set up an anonymous user for logging
	std::function<void(PMFCGI& status)> prepareAnonymous;

	//optional: extra mustache vars for login templates (TSM, theme chrome, ...)
	std::function<void(boost::json::object& json)> decorateLoginJson;

	//check the credentials, on ok also register the session and return its id
	std::function<LoginOutcome(PMFCGI& status, const QString& email, const QString& password)> login;

	//fail-fast IP/email/concurrency gate around POST login (slow KDF stall). Default on.
	bool loginLimiter = true;
	//optional: audit when the gate refuses. reason is inflight, busy, or window.
	std::function<void(PMFCGI& status, const QString& email, std::string_view reason)> onLoginRateLimited;

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

// Cookie + develLogin + hasLevel for a websocket upgrade. extractCookies first.
UpgradeAuth checkWsUpgrade(PMFCGI& status, int minLevel);

//GET renders the login form, POST (email + password [+ next]) performs the login and sets the session cookie
void loginPage(PMFCGI& status, Payload& payload);
void logout(PMFCGI& status, Payload& payload);

/**
 * The access check of the request running on this thread, for a request log
 * (BeastConf::onRequestDone). handle_request resets it at the start of each request.
 * The last check wins: a mixed route records its canRead, then its needWrite on the
 * POST branch. Set by guard, AuthAdv::guardOp / needWrite, and denyRoute.
 */
enum class CheckResult : std::uint8_t {
	none,    // no check ran (public route, static file, refused before routing)
	pass,    // the login had the level or the bit
	root,    // AuthAdv: passed as root, whatever the bits
	deny401, // not logged in
	deny403, // logged in, not allowed
};
struct LastCheck {
	int          priv   = -1; // AuthAdv Priv id; -1 = no Priv (a level guard, or no check)
	int          level  = -1; // guard<minLevel>; -1 = none
	std::uint8_t op     = 0;  // AuthAdv::Op mask (1 read, 2 write); 0 = none
	CheckResult  result = CheckResult::none;
	// The guard passed (priv / level / op are its check), then the handler refused with its own
	// denyRoute call. A project guard that is not rbk's records only its denyRoute (priv and level
	// -1), and nothing when it passes.
	bool byHandler = false;
};
[[nodiscard]] LastCheck& lastCheck(); // thread-local
void                     resetLastCheck();

//short circuit a routed handler: 401 not logged, 403 insufficient level (with audit trail hook).
//Also sets lastCheck().result to deny401 / deny403.
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
	auto& c  = conf();
	lastCheck() = LastCheck{.level = minLevel};
	//the loginManager preflight already resolved the user for every non whitelisted path
	if (!c.isLogged || !c.isLogged()) {
		denyRoute(status, payload, 401);
		return;
	}
	if (!c.hasLevel(minLevel)) {
		denyRoute(status, payload, 403);
		return;
	}
	lastCheck().result = CheckResult::pass;
	handler(status, payload);
}

} // namespace rbk::Auth

#endif // RBK_HTTP_AUTHHANDLER_H
