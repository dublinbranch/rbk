#include "rateLimit.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace rbk {

namespace {
constexpr double kNsPerMin = 60e9;

u32 wholeSeconds(double s) {
	if (!(s > 0)) {
		return 1;
	}
	const double up = std::ceil(s);
	return up >= 86400 ? 86400u : std::max(1u, static_cast<u32>(up));
}
} // namespace

i64 steadyNs() {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

double requestCap(const Limits& limits) {
	return std::max(1.0, static_cast<double>(limits.reqBurst));
}

void Bucket::refill(i64 nowNs, double perMin, double cap) {
	if (!started) {
		started = true;
		level   = cap;
		lastNs  = nowNs;
		return;
	}
	if (nowNs > lastNs) {
		level += static_cast<double>(nowNs - lastNs) * perMin / kNsPerMin;
		lastNs = nowNs;
	}
	level = std::min(level, cap);
}

double Bucket::peek(i64 nowNs, double perMin, double cap) const {
	if (!started) {
		return cap;
	}
	double l = level;
	if (nowNs > lastNs) {
		l += static_cast<double>(nowNs - lastNs) * perMin / kNsPerMin;
	}
	return std::min(l, cap);
}

bool Bucket::take(i64 nowNs, double perMin, double cap, double amount) {
	refill(nowNs, perMin, cap);
	if (level < amount) {
		return false;
	}
	level -= amount;
	return true;
}

bool Bucket::positive(i64 nowNs, double perMin, double cap) {
	refill(nowNs, perMin, cap);
	return level > 0;
}

void Bucket::debit(i64 nowNs, double perMin, double cap, double amount, double floor) {
	refill(nowNs, perMin, cap);
	level = std::max(level - amount, floor);
}

double Bucket::secondsUntil(i64 nowNs, double perMin, double cap, double needed) const {
	const double l = peek(nowNs, perMin, cap);
	if (l >= needed) {
		return 0;
	}
	if (!(perMin > 0)) {
		return 86400;
	}
	return (needed - l) / perMin * 60.0;
}

bool RateState::enter(u32 limit) noexcept {
	const auto before = inFlight.fetch_add(1, std::memory_order_acq_rel);
	if (limit > 0 && before >= limit) {
		inFlight.fetch_sub(1, std::memory_order_acq_rel);
		return false;
	}
	return true;
}

void RateState::leave() noexcept {
	inFlight.fetch_sub(1, std::memory_order_acq_rel);
}

u32 RateState::admit(i64 nowNs, const Limits& limits, Why& why, bool skipRequests) {
	why = Why::none;
	std::lock_guard guard(lock);
	if (limits.busyMsPerMin > 0) {
		const double rate = limits.busyMsPerMin;
		if (!busy.positive(nowNs, rate, rate)) {
			why = Why::busy;
			// "above 0": one ms of refill past the debt
			return wholeSeconds(busy.secondsUntil(nowNs, rate, rate, 1));
		}
	}
	if (limits.reqPerMin > 0 && !skipRequests) {
		const double rate = limits.reqPerMin;
		const double cap  = requestCap(limits);
		if (!req.take(nowNs, rate, cap)) {
			why = Why::requests;
			return wholeSeconds(req.secondsUntil(nowNs, rate, cap, 1));
		}
	}
	return 0;
}

void RateState::debit(i64 nowNs, const Limits& limits, double ms) {
	if (limits.busyMsPerMin == 0 || !(ms > 0)) {
		return;
	}
	const double    rate = limits.busyMsPerMin;
	std::lock_guard guard(lock);
	busy.debit(nowNs, rate, rate, ms, -rate);
}

void RateState::noteRefused(i64 unixNow) noexcept {
	refused.fetch_add(1, std::memory_order_relaxed);
	lastRefused.store(unixNow, std::memory_order_relaxed);
}

bool RateState::idle(i64 nowNs, const Limits& limits) const {
	if (inFlight.load(std::memory_order_acquire) != 0) {
		return false;
	}
	std::lock_guard guard(lock);
	if (limits.busyMsPerMin > 0) {
		const double rate = limits.busyMsPerMin;
		if (busy.peek(nowNs, rate, rate) < rate) {
			return false;
		}
	}
	if (limits.reqPerMin > 0) {
		const double cap = requestCap(limits);
		if (req.peek(nowNs, limits.reqPerMin, cap) < cap) {
			return false;
		}
	}
	return true;
}

RateState::View RateState::view(i64 nowNs, const Limits& limits) const {
	View v;
	{
		std::lock_guard guard(lock);
		v.reqLevel  = req.peek(nowNs, limits.reqPerMin, requestCap(limits));
		v.busyLevel = busy.peek(nowNs, limits.busyMsPerMin, limits.busyMsPerMin);
	}
	v.inFlight    = inFlight.load(std::memory_order_relaxed);
	v.refused     = refused.load(std::memory_order_relaxed);
	v.lastRefused = lastRefused.load(std::memory_order_relaxed);
	return v;
}

} // namespace rbk
