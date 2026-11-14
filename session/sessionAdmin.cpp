#include "sessionAdmin.h"

#include "sessionStore.h"

#include "rbk/BoostJson/extra.h"
#include "rbk/HTTP/PMFCGI.h"
#include "rbk/HTTP/Payload.h"
#include "rbk/HTTP/authHandler.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/minMysql/min_mysql.h"
#include "rbk/minMysql/sqlrowv2.h"

#include <boost/json.hpp>
#include <set>

namespace bj = boost::json;
using namespace std;

namespace rbk::SessionAdmin {

namespace {

bool isAdmin() {
	auto& c = conf();
	return c.store && c.db && c.isAdmin && c.isAdmin();
}

u64 currentUserId() {
	auto& c = conf();
	return c.store && c.db && c.currentUserId ? c.currentUserId() : 0;
}

u128 currentSid() {
	auto& c = conf();
	return c.currentSid ? c.currentSid() : 0;
}

void writeOk(Payload& payload, bj::object json) {
	json["status"]     = "ok";
	payload.statusCode = 200;
	payload.mime       = "application/json";
	payload.html       = pretty_print(json);
}

void writeError(Payload& payload, int code, const std::string& message) {
	rbk::Auth::writeJsonError(payload, code, message);
	payload.mime = "application/json";
}

void audit(const std::string& action, u64 userId, const bj::object& detail = {}) {
	if (auto& c = conf(); c.audit) {
		c.audit(action, userId, detail);
	}
}

// POST body {"<key>": positive integer}. 0 on anything else.
u64 bodyId(const PMFCGI& status, const char* key) {
	boost::system::error_code ec;
	auto                      v = bj::parse(status.body, ec);
	if (ec || !v.is_object()) {
		return 0;
	}
	const auto* f = v.as_object().if_contains(key);
	if (!f) {
		return 0;
	}
	if (f->is_uint64()) {
		return f->as_uint64();
	}
	if (f->is_int64() && f->as_int64() > 0) {
		return static_cast<u64>(f->as_int64());
	}
	return 0;
}

// The owner of a session row: memory first, the DB for a row that is not (or no longer) in memory.
u64 sessionOwner(u64 sessionRowId) {
	auto& c = conf();
	if (auto s = c.store->sessions.get(sessionRowId)) {
		return s->userId;
	}
	auto res = c.db->queryV2(F("SELECT userId FROM `session` WHERE id = {}", sessionRowId));
	return res.empty() ? 0 : res.at(0).rq<u64>("userId");
}

bool isCurrent(u64 sessionRowId) {
	const auto sid = currentSid();
	if (!sid) {
		return false;
	}
	auto s = conf().store->sessions.get(sessionRowId);
	return s && s->sidHash == sessionKey(sid);
}

bj::object listAnswer(u64 onlyUserId) {
	auto rows = activeSessions(onlyUserId);

	set<u64> users;
	set<u64> onlineUsers;
	u64      online = 0;
	for (const auto& v : rows) {
		const auto& o      = v.as_object();
		const auto  userId = o.at("userId").to_number<u64>();
		users.insert(userId);
		if (o.at("online").as_bool()) {
			++online;
			onlineUsers.insert(userId);
		}
	}

	bj::object json;
	json["totalSessions"]  = static_cast<u64>(rows.size());
	json["totalUsers"]     = static_cast<u64>(users.size());
	json["onlineSessions"] = online;
	json["onlineUsers"]    = static_cast<u64>(onlineUsers.size());
	json["onlineMinutes"]  = kOnlineWindowSeconds / 60;
	json["dbNow"]          = conf().store->dbNow();
	json["sessions"]       = std::move(rows);
	return json;
}

} // namespace

Conf& conf() {
	static Conf c;
	return c;
}

bj::array activeSessions(u64 onlyUserId) {
	auto&      c       = conf();
	auto&      store   = *c.store;
	const auto now     = store.dbNow();
	const auto current = currentSid() ? sessionKey(currentSid()) : u128(0);

	auto res = c.db->queryV2(F(R"(
SELECT id, userId, createdAt, expireAt, createdIp, userAgent
FROM `session`
WHERE revoked = 0 AND expireAt > UNIX_TIMESTAMP(){}
ORDER BY id DESC)",
	                           onlyUserId ? F(" AND userId = {}", onlyUserId) : string()));

	bj::array out;
	out.reserve(static_cast<size_t>(res.size()));
	for (const auto& r : res) {
		const auto id     = r.rq<u64>("id");
		const auto userId = r.rq<u64>("userId");
		bj::object o;
		o["id"]        = id;
		o["userId"]    = userId;
		o["createdAt"] = r.rq<i64>("createdAt");
		o["expireAt"]  = r.rq<i64>("expireAt");
		o["createdIp"] = r.rq<string>("createdIp");
		o["userAgent"] = r.rq<string>("userAgent");
		if (c.describeUser) {
			c.describeUser(o, userId);
		}

		// Live counters: memory only, since the last restart.
		i64  lastSeen  = 0;
		u64  pageViews = 0;
		bool thisOne   = false;
		o["lastIp"]    = "";
		if (auto s = store.sessions.get(id); s && s->rt) {
			lastSeen  = s->rt->lastSeen.load();
			pageViews = s->rt->pageViews.load();
			if (auto ip = s->rt->lastIp.load()) {
				o["lastIp"] = *ip;
			}
			thisOne   = current != 0 && s->sidHash == current;
		}
		o["lastSeen"]  = lastSeen;
		o["pageViews"] = pageViews;
		o["online"]    = lastSeen > 0 && now - lastSeen <= kOnlineWindowSeconds;
		o["current"]   = thisOne;
		out.push_back(std::move(o));
	}
	return out;
}

void listApi(PMFCGI& status, Payload& payload) {
	if (!isAdmin()) {
		writeError(payload, 403, "insufficient privileges");
		return;
	}
	u64 onlyUserId = 0;
	if (auto uid = status.get.get("userId"); uid) {
		onlyUserId = uid.val->toULongLong();
	}
	writeOk(payload, listAnswer(onlyUserId));
}

void revokeSessionApi(PMFCGI& status, Payload& payload) {
	if (!isAdmin()) {
		writeError(payload, 403, "insufficient privileges");
		return;
	}
	const auto id = bodyId(status, "id");
	if (!id) {
		writeError(payload, 400, "id required");
		return;
	}
	const auto owner = sessionOwner(id);
	if (!owner) {
		writeError(payload, 404, "session not found");
		return;
	}
	const bool self    = isCurrent(id);
	const bool revoked = conf().store->revokeRow(*conf().db, id, SessionStore::EndReason::revoked);
	if (revoked) {
		bj::object detail;
		detail["sessionId"] = id;
		audit("revokeSession", owner, detail);
	}
	bj::object o;
	o["revoked"] = revoked;
	o["self"]    = self;
	writeOk(payload, std::move(o));
}

void revokeUserSessionsApi(PMFCGI& status, Payload& payload) {
	if (!isAdmin()) {
		writeError(payload, 403, "insufficient privileges");
		return;
	}
	const auto userId = bodyId(status, "userId");
	if (!userId) {
		writeError(payload, 400, "userId required");
		return;
	}
	// Never log the admin out of the browser that clicked the button.
	const u128 keep  = userId == currentUserId() ? currentSid() : 0;
	const auto count = conf().store->revokeUser(*conf().db, userId, keep, SessionStore::EndReason::revokedUser);
	bj::object detail;
	detail["count"] = static_cast<u64>(count);
	audit("revokeUserSessions", userId, detail);
	bj::object o;
	o["revoked"] = static_cast<u64>(count);
	writeOk(payload, std::move(o));
}

void mySessionsApi(PMFCGI& status, Payload& payload) {
	(void)status;
	const auto me = currentUserId();
	if (!me) {
		writeError(payload, 401, "login required");
		return;
	}
	writeOk(payload, listAnswer(me));
}

void revokeMySessionApi(PMFCGI& status, Payload& payload) {
	const auto me = currentUserId();
	if (!me) {
		writeError(payload, 401, "login required");
		return;
	}
	const auto id = bodyId(status, "id");
	if (!id) {
		writeError(payload, 400, "id required");
		return;
	}
	// The same answer for "not yours" and "does not exist": do not tell the session ids of others.
	if (sessionOwner(id) != me) {
		writeError(payload, 404, "session not found");
		return;
	}
	if (isCurrent(id)) {
		writeError(payload, 400, "this is the current session, use log out");
		return;
	}
	const bool revoked = conf().store->revokeRow(*conf().db, id, SessionStore::EndReason::revoked);
	if (revoked) {
		bj::object detail;
		detail["sessionId"] = id;
		audit("revokeMySession", me, detail);
	}
	bj::object o;
	o["revoked"] = revoked;
	writeOk(payload, std::move(o));
}

void revokeMyOtherSessionsApi(PMFCGI& status, Payload& payload) {
	(void)status;
	const auto me = currentUserId();
	if (!me) {
		writeError(payload, 401, "login required");
		return;
	}
	// No current session (a token): keep nothing, there is no browser to keep logged in.
	const auto count = conf().store->revokeUser(*conf().db, me, currentSid(), SessionStore::EndReason::revokedUser);
	if (count) {
		bj::object detail;
		detail["count"] = static_cast<u64>(count);
		audit("revokeMyOtherSessions", me, detail);
	}
	bj::object o;
	o["revoked"] = static_cast<u64>(count);
	writeOk(payload, std::move(o));
}

} // namespace rbk::SessionAdmin
