#ifndef RBK_RATELIMIT_RATELIMIT_H
#define RBK_RATELIMIT_RATELIMIT_H

// Rate limits in memory: token buckets on the steady clock, and the state of one limited key (a
// login, an address). No DB, no Qt, no lock order with anything else: each RateState has its own
// leaf mutex. See README.md next to this file.

#include "rbk/number/intTypes.h"

#include <atomic>
#include <mutex>

namespace rbk {

/** The limits of one key. A bucket with a rate of 0 has no limit. */
struct Limits {
	u32 reqPerMin    = 0; // refill of the request bucket; 0 = no limit
	u32 reqBurst     = 0; // size of the request bucket; taken as at least 1 when reqPerMin > 0
	u32 busyMsPerMin = 0; // refill of the busy bucket, its size is one minute of it; 0 = no limit
	u32 inFlight     = 0; // requests of the key at the same time; 0 = no limit

	bool operator==(const Limits&) const = default;
};

/** Monotonic ns, the clock of the buckets. */
[[nodiscard]] i64 steadyNs();

/**
 * One token bucket: `level` fills at `perMin` units a minute up to `cap`. A new bucket starts full.
 * Not thread safe: the owner locks. A clock that goes back adds nothing.
 */
class Bucket {
      public:
	/** Refill, then take `amount` if the level has it. */
	bool take(i64 nowNs, double perMin, double cap, double amount = 1);
	/** Refill, then true if the level is above 0 (takes nothing). */
	bool positive(i64 nowNs, double perMin, double cap);
	/** Refill, then subtract `amount`, never below `floor` (a debt, paid back by the refill). */
	void debit(i64 nowNs, double perMin, double cap, double amount, double floor);
	/** Seconds until the level reaches `needed`; 0 when it is there. */
	[[nodiscard]] double secondsUntil(i64 nowNs, double perMin, double cap, double needed) const;
	/** The level a refill would give now, without changing the bucket. */
	[[nodiscard]] double peek(i64 nowNs, double perMin, double cap) const;

      private:
	void   refill(i64 nowNs, double perMin, double cap);
	double level   = 0;
	i64    lastNs  = 0;
	bool   started = false;
};

/**
 * The state of one limited key: a request bucket, a busy bucket (ms of wall time), the requests in
 * flight, the refusals. Thread safe. A request does:
 *
 *   enter(limits.inFlight)    -> false: refuse (Retry-After 1); true: a slot is held, leave() it
 *   admit(now, limits)        -> 0: go on; else refuse with that Retry-After (and leave() the slot)
 *   ... the request ...
 *   debit(now, limits, ms)    the wall time of the request
 *   leave()
 */
class RateState {
      public:
	enum class Why : u8 {
		none,
		inFlight,
		requests,
		busy,
	};
	struct View {
		double reqLevel    = 0; // tokens now (meaningless with no limit)
		double busyLevel   = 0; // ms now, below 0 = a debt
		u32    inFlight    = 0;
		u64    refused     = 0;
		i64    lastRefused = 0; // unix s, 0 = never
	};

	/** Counts the request in flight. With a limit above 0 and that many already in: false, nothing
	 *  counted. Lock free. */
	[[nodiscard]] bool enter(u32 limit) noexcept;
	void               leave() noexcept;

	/** The busy bucket, then the request bucket (so a request refused for busy takes no token).
	 *  0 = admitted (a token taken), else Retry-After in whole seconds, at least 1; `why` says
	 *  which. skipRequests: the request bucket is not used for this request. */
	[[nodiscard]] u32 admit(i64 nowNs, const Limits& limits, Why& why, bool skipRequests = false);
	/** The wall time of one request, in ms. The busy level never goes below -busyMsPerMin. */
	void debit(i64 nowNs, const Limits& limits, double ms);
	/** A refusal: counted, with its time (unix s). */
	void noteRefused(i64 unixNow) noexcept;

	/** Both buckets full and nothing in flight: the state can be dropped and made again. */
	[[nodiscard]] bool idle(i64 nowNs, const Limits& limits) const;
	[[nodiscard]] View view(i64 nowNs, const Limits& limits) const;

      private:
	mutable std::mutex lock;
	Bucket             req;
	Bucket             busy;
	std::atomic<u32>   inFlight{0};
	std::atomic<u64>   refused{0};
	std::atomic<i64>   lastRefused{0};
};

/** The cap of the request bucket: the burst, at least 1 (a burst of 0 with a rate refuses nothing
 *  for good). */
[[nodiscard]] double requestCap(const Limits& limits);

} // namespace rbk

#endif // RBK_RATELIMIT_RATELIMIT_H
