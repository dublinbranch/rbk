#include "sessionStore.h"

#include "rbk/QStacker/exceptionv2.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/minMysql/min_mysql.h"
#include "rbk/minMysql/sqlrowv2.h"
#include "rbk/rand/secureRandom.h"

#include <QCryptographicHash>
#include <QDebug>
#include <arpa/inet.h>
#include <array>
#include <mutex>
#include <set>

using namespace std;

namespace rbk {

void SessionRow::fromSql(const SqlRowV2& r) {
	r.rq("id", id);
	const auto hi = r.rq<u64>("sidHi");
	const auto lo = r.rq<u64>("sidLo");
	sidHash       = (u128(hi) << 64) | u128(lo);
	r.rq("userId", userId);
	r.rq("expireAt", expireAt);
	r.rq("revoked", revoked);
}

string sanitizeUtf8(string_view s, size_t maxBytes) {
	string out;
	out.reserve(min(s.size(), maxBytes));
	size_t i = 0;
	while (i < s.size()) {
		const auto c   = static_cast<unsigned char>(s[i]);
		size_t     len = 0;
		u32        cp  = 0;
		if (c < 0x80) {
			len = 1;
			cp  = c;
		} else if ((c & 0xE0) == 0xC0) {
			len = 2;
			cp  = c & 0x1F;
		} else if ((c & 0xF0) == 0xE0) {
			len = 3;
			cp  = c & 0x0F;
		} else if ((c & 0xF8) == 0xF0) {
			len = 4;
			cp  = c & 0x07;
		} else {
			++i; // stray continuation or invalid lead byte
			continue;
		}
		if (i + len > s.size()) {
			break; // truncated sequence at the end
		}
		bool valid = true;
		for (size_t k = 1; k < len; ++k) {
			const auto cc = static_cast<unsigned char>(s[i + k]);
			if ((cc & 0xC0) != 0x80) {
				valid = false;
				break;
			}
			cp = (cp << 6) | (cc & 0x3F);
		}
		// Reject overlong forms, surrogates, and > U+10FFFF.
		static constexpr u32 minCp[5] = {0, 0, 0x80, 0x800, 0x10000};
		if (!valid || cp < minCp[len] || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
			++i;
			continue;
		}
		if (cp < 0x20 || cp == 0x7F) {
			i += len; // control char
			continue;
		}
		if (out.size() + len > maxBytes) {
			break; // never split a char
		}
		out.append(s.substr(i, len));
		i += len;
	}
	return out;
}

string sanitizeIp(string_view s) {
	if (s.empty() || s.size() > 45) {
		return {};
	}
	const string    ip(s);
	array<char, 16> buf{};
	if (inet_pton(AF_INET, ip.c_str(), buf.data()) == 1 || inet_pton(AF_INET6, ip.c_str(), buf.data()) == 1) {
		return ip;
	}
	return {};
}

string sidToHex(u128 sid) {
	static constexpr char digits[] = "0123456789abcdef";
	string                out(32, '0');
	for (int i = 31; i >= 0; --i) {
		out[size_t(i)] = digits[size_t(sid & 0xF)];
		sid >>= 4;
	}
	return out;
}

u128 sessionKey(u128 sid) {
	array<char, 16> bytes{};
	for (int i = 15; i >= 0; --i) {
		bytes[size_t(i)] = char(uint8_t(sid & 0xFF));
		sid >>= 8;
	}
	const auto digest = QCryptographicHash::hash(QByteArrayView(bytes.data(), 16), QCryptographicHash::Sha256);
	u128       key    = 0;
	for (int i = 0; i < 16; ++i) {
		key = (key << 8) | u128(uint8_t(digest[i]));
	}
	return key;
}

optional<u128> sidFromHex(string_view hex) {
	if (hex.size() != 32) {
		return nullopt;
	}
	u128 v = 0;
	for (char c : hex) {
		u128 d = 0;
		if (c >= '0' && c <= '9') {
			d = u128(c - '0');
		} else if (c >= 'a' && c <= 'f') {
			d = u128(c - 'a' + 10);
		} else {
			return nullopt;
		}
		v = (v << 4) | d;
	}
	if (v == 0) {
		return nullopt;
	}
	return v;
}

namespace {

u128 newSid() {
	for (;;) {
		array<uint8_t, 16> bytes{};
		secureRandomBytes(bytes);
		u128 v = 0;
		for (auto b : bytes) {
			v = (v << 8) | b;
		}
		if (v != 0) {
			return v;
		}
	}
}

u64 sidHi(u128 sid) {
	return u64(sid >> 64);
}

u64 sidLo(u128 sid) {
	return u64(sid);
}

} // namespace

SessionStore::SessionStore() {
	sessions.selectCols = "id, sidHi, sidLo, userId, expireAt, revoked";
	sessions.aliveWhere = "revoked = 0 AND expireAt > UNIX_TIMESTAMP()";
	// Runs when a row enters the map, never during construction, so the virtual call is safe.
	// An expired row read again by the delta overlap must not come back (it would end twice).
	sessions.admitNew = [this](SessionRow& row) {
		if (row.expireAt < dbNow()) {
			return false;
		}
		if (!row.rt) {
			row.rt = makeRuntime();
		}
		return true;
	};
}

shared_ptr<SessionRuntime> SessionStore::makeRuntime() {
	return make_shared<SessionRuntime>();
}

void SessionStore::onSessionEnd(const SessionPtr&, EndReason) {
}

void SessionStore::fireEnd(const vector<SessionPtr>& ended, EndReason reason) {
	for (const auto& s : ended) {
		try {
			onSessionEnd(s, reason);
		} catch (const std::exception& e) {
			qWarning().noquote() << F("SessionStore::onSessionEnd threw for session {}: {}", s->id, e.what());
		} catch (...) {
			qWarning().noquote() << F("SessionStore::onSessionEnd threw for session {}", s->id);
		}
	}
}

namespace {
i64 steadySeconds() {
	return chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now().time_since_epoch()).count();
}
} // namespace

void SessionStore::advanceDbNow(i64 now) {
	// The last read wins, not the largest: a DB clock that jumped forward and came back (a failover)
	// must not stay ahead until a restart. Reads of two threads differ by a second at most.
	dbOffset.store(now - steadySeconds(), memory_order_relaxed);
}

i64 SessionStore::dbNow() const {
	const auto offset = dbOffset.load(memory_order_relaxed);
	return offset ? steadySeconds() + offset : 0;
}

i64 SessionStore::selectDbNow(DB& db) {
	auto       res = db.queryV2("SELECT UNIX_TIMESTAMP() AS now");
	const auto now = res.at(0).rq<i64>("now");
	advanceDbNow(now);
	return now;
}

void SessionStore::boot(DB& db) {
	selectDbNow(db); // before loading: admitNew compares expireAt with it
	loadUsers(db);
	sessions.loadAll(db);
	selectDbNow(db);
}

void SessionStore::refresh(DB& db) {
	selectDbNow(db);
	refreshUsers(db);
	fireEnd(sessions.refresh(db), EndReason::refreshErase);

	// The sessions of disabled users. Over the sessions, not the users: a project can have many
	// rows that never log in. Collect first, then revoke: no map lock while waiting on a writeGate.
	// An unknown user is only refused (resolve), not revoked: a bad load must not end every session.
	set<u64> disabled;
	for (const auto& s : sessions.all()) {
		if (!disabled.contains(s->userId) && userState(s->userId) == UserState::disabled) {
			disabled.insert(s->userId);
		}
	}
	for (auto userId : disabled) {
		revokeUser(db, userId, 0, EndReason::userDisabled);
	}

	// Expiry sweep.
	vector<SessionPtr> expired;
	{
		lock_guard gate(sessions.writeGate);
		const auto now = selectDbNow(db);
		auto       lk  = sessions.map.uniqueLock();
		auto&      idx = sessions.map.container.get<ByExpire>();
		while (!idx.empty() && (*idx.begin())->expireAt < now) {
			expired.push_back(*idx.begin());
			idx.erase(idx.begin());
		}
	}
	fireEnd(expired, EndReason::expired);

	// A row deleted by hand leaves no updatedAt for the delta: when the DB has fewer live rows than
	// memory, reload the sessions now instead of at the next reconcile. A row that expired between
	// the sweep and this count also triggers it: harmless, the reload is small.
	const auto live = db.queryV2("SELECT COUNT(*) AS n FROM `session` WHERE revoked = 0 AND expireAt >= UNIX_TIMESTAMP()")
	                      .at(0)
	                      .rq<u64>("n");
	size_t inMemory = 0;
	{
		auto lk  = sessions.map.sharedLock();
		inMemory = sessions.map.container.size();
	}
	if (live < inMemory) {
		fireEnd(sessions.reconcile(db), EndReason::reconcileErase);
	}
}

void SessionStore::reconcile(DB& db) {
	reconcileUsers(db);
	fireEnd(sessions.reconcile(db), EndReason::reconcileErase);
	selectDbNow(db);
}

void SessionStore::cleanup(DB& db) {
	db.query(R"(
DELETE FROM `session`
WHERE expireAt < UNIX_TIMESTAMP() - 86400
   OR (revoked = 1 AND updatedAt < NOW() - INTERVAL 1 DAY))");
}

void SessionStore::maintain(DB& db) {
	refresh(db);
	const auto now = chrono::steady_clock::now();
	if (now - lastReconcile >= reconcileEvery) {
		reconcile(db);
		lastReconcile = now;
	}
	if (lastCleanup == chrono::steady_clock::time_point{} || now - lastCleanup >= cleanupEvery) {
		cleanup(db);
		lastCleanup = now;
	}
}

u128 SessionStore::create(DB& db, u64 userId, i64 ttl, string_view ip, string_view userAgent) {
	if (!userId) {
		throw ExceptionV2("SessionStore::create: user id is 0");
	}

	const auto sid = newSid();
	lock_guard gate(sessions.writeGate);
	const auto now = selectDbNow(db);

	// Client-controlled: strict mode rejects invalid UTF-8 or a split char, and login would fail.
	const auto cleanIp = sanitizeIp(ip);
	const auto cleanUa = sanitizeUtf8(userAgent, 255);
	const auto key     = sessionKey(sid);
	db.query(F("INSERT INTO `session` (sidHi, sidLo, userId, createdAt, expireAt, createdIp, userAgent) "
	           "VALUES ({}, {}, {}, {}, {}, '{}', '{}')",
	           sidHi(key), sidLo(key), userId, now, now + ttl, db.escape(cleanIp), db.escape(cleanUa)));

	auto row      = make_shared<SessionRow>();
	row->id       = db.lastId();
	row->sidHash  = key;
	row->userId   = userId;
	row->expireAt = now + ttl;
	sessions.upsert(std::move(row));
	return sid;
}

SessionPtr SessionStore::resolve(string_view cookie) const {
	if (auto sid = sidFromHex(cookie); sid) {
		return resolve(*sid);
	}
	return {};
}

SessionPtr SessionStore::resolve(u128 sid) const {
	auto s = sessions.find<BySid>(sessionKey(sid));
	if (!s || s->revoked || s->expireAt < dbNow()) {
		return {};
	}
	// After the map lock is released: the hook takes the lock of the project's user table.
	if (userState(s->userId) != UserState::ok) {
		return {};
	}
	return s;
}

void SessionStore::touch(const SessionPtr& s, i64 now, string_view remoteIp) {
	if (!s || !s->rt) {
		return;
	}
	s->rt->pageViews.fetch_add(1, memory_order_relaxed);
	s->rt->lastSeen.store(now, memory_order_relaxed);
	if (auto cur = s->rt->lastIp.load(); !cur || *cur != remoteIp) { // no allocation when the IP did not change
		s->rt->lastIp.store(make_shared<const string>(remoteIp));
	}
}

bool SessionStore::revoke(DB& db, u128 sid, EndReason reason) {
	SessionPtr ended;
	{
		lock_guard gate(sessions.writeGate);
		const auto key = sessionKey(sid);
		db.query(F("UPDATE `session` SET revoked = 1 WHERE sidHi = {} AND sidLo = {} AND revoked = 0", sidHi(key),
		           sidLo(key)));
		if (auto s = sessions.find<BySid>(key)) {
			ended = sessions.erase(s->id);
		}
		selectDbNow(db);
	}
	if (ended) {
		fireEnd({ended}, reason);
	}
	return ended != nullptr;
}

bool SessionStore::revokeRow(DB& db, u64 sessionRowId, EndReason reason) {
	SessionPtr ended;
	{
		lock_guard gate(sessions.writeGate);
		db.query(F("UPDATE `session` SET revoked = 1 WHERE id = {} AND revoked = 0", sessionRowId));
		ended = sessions.erase(sessionRowId);
		selectDbNow(db);
	}
	if (ended) {
		fireEnd({ended}, reason);
	}
	return ended != nullptr;
}

size_t SessionStore::revokeUser(DB& db, u64 userId, u128 keepSid, EndReason reason) {
	if (!userId) {
		return 0;
	}
	vector<SessionPtr> ended;
	{
		lock_guard gate(sessions.writeGate);
		// 0 keeps nothing: no key is 0 in practice, and none is written as 0
		const u128 keep = keepSid ? sessionKey(keepSid) : 0;
		// DB first: also covers rows that are not in memory.
		db.query(F("UPDATE `session` SET revoked = 1 WHERE userId = {} AND revoked = 0 AND NOT (sidHi = {} AND sidLo = {})",
		           userId, sidHi(keep), sidLo(keep)));
		for (const auto& s : sessions.findAll<ByUser>(userId)) {
			if (keep != 0 && s->sidHash == keep) {
				continue;
			}
			if (auto old = sessions.erase(s->id)) {
				ended.push_back(std::move(old));
			}
		}
		selectDbNow(db);
	}
	fireEnd(ended, reason);
	return ended.size();
}

vector<SessionPtr> SessionStore::sessionsOf(u64 userId) const {
	return sessions.findAll<ByUser>(userId);
}

} // namespace rbk
