#ifndef RBK_SESSION_SESSIONSTORE_H
#define RBK_SESSION_SESSIONSTORE_H

// Login sessions in the DB, kept in memory by rbk::LiveTable. No DB read and no DB write per
// request. Moved from the PPPLC CMMS (sessionStore.*), so the CMMS, digitalSpine and diter share it.
// See README.md next to this file.

#include "rbk/liveTable/liveTable.h"
#include "rbk/number/intTypes.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class DB;
class SqlRowV2;

namespace rbk {

/** Live, never in the DB, reset on a restart. A project store can add members (makeRuntime). */
struct SessionRuntime {
	virtual ~SessionRuntime() = default;
	std::atomic<u64>                                pageViews{0};
	std::atomic<i64>                                lastSeen{0}; // DB clock (dbNow) at the last request
	std::atomic<std::shared_ptr<const std::string>> lastIp;
};

struct SessionRow {
	static constexpr std::string_view table    = "session";
	u64                               id       = 0;
	// sessionKey(sid): the DB and memory hold the hash, never the sid (the cookie). Someone who
	// reads the table cannot rebuild a cookie from it. sidHi << 64 | sidLo.
	u128 sidHash = 0;
	u64                               userId   = 0;
	i64                               expireAt = 0;
	bool                              revoked  = false;
	// The pointer is const in a const row, the block is not. From makeRuntime(), never from SQL.
	std::shared_ptr<SessionRuntime> rt;

	void fromSql(const SqlRowV2& r);
	bool alive() const {
		return !revoked;
	}
	void adopt(const SessionRow& old) {
		rt = old.rt;
	}
	// DB fields only, never rt.
	bool operator==(const SessionRow& o) const {
		return id == o.id && sidHash == o.sidHash && userId == o.userId && expireAt == o.expireAt && revoked == o.revoked;
	}
};
using SessionPtr = std::shared_ptr<const SessionRow>;

struct BySid {};
struct ByUser {};
struct ByExpire {};

struct SidHash {
	size_t operator()(u128 v) const noexcept {
		return size_t(u64(v)) ^ size_t(u64(v >> 64));
	}
};

/** 32 lowercase hex chars: the cookie value. */
[[nodiscard]] std::string sidToHex(u128 sid);
/** The first 128 bits of SHA-256 of the 16 bytes of the sid (big endian): what the DB and the
 *  memory index hold. The sid is 128 random bits, so a hash with no salt is enough. */
[[nodiscard]] u128 sessionKey(u128 sid);
/** Strict: exactly 32 lowercase hex chars and not 0, else nullopt. */
[[nodiscard]] std::optional<u128> sidFromHex(std::string_view hex);
/** Client text for a utf8mb4 column: valid UTF-8 only (bad bytes dropped), no control chars, at most
 *  maxBytes, never a split char. */
[[nodiscard]] std::string sanitizeUtf8(std::string_view s, size_t maxBytes);
/** Client IP for an ascii column: the input if it parses as IPv4 / IPv6 (inet_pton), else empty. */
[[nodiscard]] std::string sanitizeIp(std::string_view s);

/**
 * The sessions. The users are the project's: a subclass keeps its own user table (a LiveTable on
 * its user row) and answers userState() from memory. rbk never reads the user table.
 *
 * The table, for the next project:
 *
 *   CREATE TABLE `session` (
 *     `id` bigint unsigned NOT NULL AUTO_INCREMENT,
 *     `sidHi` bigint unsigned NOT NULL, `sidLo` bigint unsigned NOT NULL,  -- sessionKey(sid), never the sid
 *     `userId` int unsigned NOT NULL,
 *     `createdAt` bigint NOT NULL, `expireAt` bigint NOT NULL,             -- unix s, DB clock
 *     `revoked` tinyint unsigned NOT NULL DEFAULT 0,
 *     `createdIp` varchar(45) CHARACTER SET ascii COLLATE ascii_bin NOT NULL DEFAULT '',
 *     `userAgent` varchar(255) NOT NULL DEFAULT '',
 *     `updatedAt` timestamp(3) NOT NULL DEFAULT current_timestamp(3) ON UPDATE current_timestamp(3),
 *     PRIMARY KEY (`id`), UNIQUE KEY `sid` (`sidHi`,`sidLo`), KEY `userId` (`userId`),
 *     KEY `updatedAt` (`updatedAt`));
 *
 * Locks: each LiveTable has its own (writeGate -> map). The user hooks are called with no lock of
 * the sessions held.
 */
class SessionStore {
      public:
	enum class EndReason {
		logout,
		revoked,
		revokedUser,
		userDisabled,
		expired,
		refreshErase,
		reconcileErase,
	};

	// What the project knows of a user, from memory.
	enum class UserState {
		unknown,  // not in memory: its sessions are refused, not revoked (a reconcile may bring it back)
		ok,       // may log in
		disabled, // known and may not log in: refresh revokes its sessions in the DB
	};

	SessionStore();
	virtual ~SessionStore() = default;

	SessionStore(const SessionStore&)            = delete;
	SessionStore& operator=(const SessionStore&) = delete;

	/** loadUsers -> sessions.loadAll -> dbNow. Before the first request. */
	void boot(DB& db);
	/** Every few s: refreshUsers, sessions delta, revoke the sessions of disabled users, expiry sweep,
	 *  and a count that catches a row deleted by hand. */
	void refresh(DB& db);
	/** Every N min: reconcileUsers, full sessions reload (the safety net of a missed delta). */
	void reconcile(DB& db);
	/** Once a day: hard delete of old expired and revoked session rows. */
	void cleanup(DB& db);
	/** Call every few seconds from ONE thread: refresh, plus reconcile / cleanup when due.
	 *  Cleanup is due on the first call, so a server restarted daily still cleans up. */
	void                                maintain(DB& db);
	std::chrono::steady_clock::duration reconcileEvery = std::chrono::minutes(10);
	std::chrono::steady_clock::duration cleanupEvery   = std::chrono::hours(24);

	/** A new session for userId. The caller checked the password and put the user in memory: this
	 *  does not look at the user. Throws on userId 0. */
	[[nodiscard]] u128 create(DB& db, u64 userId, i64 ttl, std::string_view ip, std::string_view userAgent);

	/** The session, if it is alive and its user is ok (memory only, no DB). */
	[[nodiscard]] SessionPtr resolve(std::string_view cookie) const;
	[[nodiscard]] SessionPtr resolve(u128 sid) const;
	/** Last seen and last IP of the session, in memory. */
	static void touch(const SessionPtr& s, i64 now, std::string_view remoteIp);

	bool   revoke(DB& db, u128 sid, EndReason reason = EndReason::logout);
	bool   revokeRow(DB& db, u64 sessionRowId, EndReason reason = EndReason::revoked);
	size_t revokeUser(DB& db, u64 userId, u128 keepSid = 0, EndReason reason = EndReason::revokedUser);

	[[nodiscard]] std::vector<SessionPtr> sessionsOf(u64 userId) const;
	[[nodiscard]] UserState               stateOf(u64 userId) const {
		return userState(userId);
	}

	/** The DB clock: the last value read from the DB, moved on by the steady clock since. It keeps
	 *  running while the DB is down, so sessions still expire then. */
	void              advanceDbNow(i64 now);
	[[nodiscard]] i64 dbNow() const;

	rbk::LiveTable<SessionRow, SessionStore,
	               boost::multi_index::hashed_unique<boost::multi_index::tag<BySid>, rbk::ptrMember<&SessionRow::sidHash>, SidHash>,
	               boost::multi_index::hashed_non_unique<boost::multi_index::tag<ByUser>, rbk::ptrMember<&SessionRow::userId>>,
	               boost::multi_index::ordered_non_unique<boost::multi_index::tag<ByExpire>, rbk::ptrMember<&SessionRow::expireAt>>>
	    sessions;

      protected:
	// The user side. Called by boot / refresh / reconcile before the sessions, never from the
	// constructor. Each may throw: the call stops, and the next tick tries again.
	virtual void loadUsers(DB& db)      = 0;
	virtual void refreshUsers(DB& db)   = 0;
	virtual void reconcileUsers(DB& db) = 0;
	// Memory only, no DB, cheap: it runs on each request. No lock of this store is held.
	[[nodiscard]] virtual UserState userState(u64 userId) const = 0;

	// Extension points. Base = counters only / no-op. Never called from the constructor.
	// makeRuntime runs under the sessions map lock: cheap, no DB, no other lock.
	virtual std::shared_ptr<SessionRuntime> makeRuntime();
	// Called after every lock is released, on the thread that removed the session.
	// An exception is logged and swallowed: the other sessions still get their call.
	virtual void onSessionEnd(const SessionPtr& session, EndReason reason);

      private:
	void fireEnd(const std::vector<SessionPtr>& ended, EndReason reason);
	i64  selectDbNow(DB& db);

	// dbNow() = steady seconds + this; 0 before the first read
	std::atomic<i64> dbOffset{0};
	// maintain() only (one thread)
	std::chrono::steady_clock::time_point lastReconcile = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point lastCleanup{}; // epoch: due on the first call
};

} // namespace rbk

#endif // RBK_SESSION_SESSIONSTORE_H
