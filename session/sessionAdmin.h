#ifndef RBK_SESSION_SESSIONADMIN_H
#define RBK_SESSION_SESSIONADMIN_H

// The routes that list and end sessions, on rbk::SessionStore. JSON answers; the page is the
// project's. Never shows the sid (the cookie value): rows use session.id only.
// From the PPPLC CMMS (cloud/sessionAdmin.*).

#include "rbk/number/intTypes.h"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <functional>
#include <string>

class PMFCGI;
class Payload;
class DB;

namespace rbk {
class SessionStore;
}

namespace rbk::SessionAdmin {

// A session counts as "online" when its last request is at most this old (DB clock).
inline constexpr i64 kOnlineWindowSeconds = 300;

struct Conf {
	SessionStore* store = nullptr;
	DB*           db    = nullptr;
	// Each handler checks its own caller too, so a wrong route fails closed. A hook left empty = no.
	std::function<bool()> isAdmin;       // list, revokeSession, revokeUserSessions
	std::function<u64()>  currentUserId; // 0 = not logged in
	std::function<u128()> currentSid;    // 0 = no session (a token, a request with no cookie)
	// The user columns of a row: email, level, ... From memory (the project's user table).
	std::function<void(boost::json::object& row, u64 userId)> describeUser;
	// action: revokeSession, revokeUserSessions, revokeMySession, revokeMyOtherSessions.
	std::function<void(const std::string& action, u64 userId, const boost::json::object& detail)> audit;
};

// the singleton filled by the project at startup, before the server starts
Conf& conf();

/** Active sessions (not revoked, not expired), newest first. onlyUserId 0 = every user. The DB has
 *  createdAt, createdIp, userAgent; memory has lastSeen, lastIp, pageViews (since the last restart). */
[[nodiscard]] boost::json::array activeSessions(u64 onlyUserId);

// Admin. GET ?userId= (optional): {status, sessions, totalSessions, totalUsers, onlineSessions,
// onlineUsers, onlineMinutes, dbNow}.
void listApi(PMFCGI& status, Payload& payload);
// Admin. POST {"id": session row id}.
void revokeSessionApi(PMFCGI& status, Payload& payload);
// Admin. POST {"userId": id}. Keeps the caller's own current session.
void revokeUserSessionsApi(PMFCGI& status, Payload& payload);

// Any login, its own sessions only. GET: the same answer as listApi.
void mySessionsApi(PMFCGI& status, Payload& payload);
// POST {"id": session row id}. The current session is refused (use log out).
void revokeMySessionApi(PMFCGI& status, Payload& payload);
// POST: every own session but the current one.
void revokeMyOtherSessionsApi(PMFCGI& status, Payload& payload);

} // namespace rbk::SessionAdmin

#endif // RBK_SESSION_SESSIONADMIN_H
