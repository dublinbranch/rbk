#include "loginLimiter.h"

#include <QString>
#include <algorithm>

using namespace std::chrono;

namespace rbk::Auth {

LoginLimiter::Guard::Guard(LoginLimiter* owner, std::string ip, std::string email)
    : owner_(owner), ip_(std::move(ip)), email_(std::move(email)) {
}

LoginLimiter::Guard::Guard(Guard&& other) noexcept
    : owner_(other.owner_), ip_(std::move(other.ip_)), email_(std::move(other.email_)) {
	other.owner_ = nullptr;
}

LoginLimiter::Guard& LoginLimiter::Guard::operator=(Guard&& other) noexcept {
	if (this != &other) {
		reset();
		owner_       = other.owner_;
		ip_          = std::move(other.ip_);
		email_       = std::move(other.email_);
		other.owner_ = nullptr;
	}
	return *this;
}

LoginLimiter::Guard::~Guard() {
	reset();
}

void LoginLimiter::Guard::reset() {
	if (!owner_) {
		return;
	}
	owner_->release(ip_, email_);
	owner_ = nullptr;
}

LoginLimiter::LoginLimiter(Clock clock)
    : clock_(std::move(clock)) {
	if (!clock_) {
		clock_ = [] {
			return steady_clock::now();
		};
	}
}

LoginLimiter& LoginLimiter::instance() {
	static LoginLimiter limiter;
	return limiter;
}

std::string LoginLimiter::normalizeEmail(std::string_view email) {
	return QString::fromUtf8(email.data(), static_cast<qsizetype>(email.size()))
	        .trimmed()
	        .toLower()
	        .toStdString();
}

const char* LoginLimiter::admitReason(Admit admit) {
	switch (admit) {
	case Admit::inflight:
		return "inflight";
	case Admit::busy:
		return "busy";
	case Admit::throttled:
		return "window";
	case Admit::ok:
		return "ok";
	}
	return "window";
}

steady_clock::time_point LoginLimiter::now() const {
	return clock_();
}

void LoginLimiter::pruneLocked(std::vector<steady_clock::time_point>& stamps,
                               steady_clock::time_point               now,
                               steady_clock::duration                 window) const {
	const auto cutoff = now - window;
	stamps.erase(std::remove_if(stamps.begin(), stamps.end(),
	                            [cutoff](steady_clock::time_point t) {
		                            return t < cutoff;
	                            }),
	             stamps.end());
}

unsigned LoginLimiter::remainingSec(const std::vector<steady_clock::time_point>& stamps,
                                    steady_clock::time_point                     now,
                                    steady_clock::duration                       window) const {
	if (stamps.empty()) {
		return kInflightRetryAfterSec;
	}
	const auto until = stamps.front() + window;
	if (until <= now) {
		return 1;
	}
	const auto sec = duration_cast<seconds>(until - now).count();
	return static_cast<unsigned>(sec < 1 ? 1 : sec);
}

LoginLimiter::TryBegin LoginLimiter::tryBegin(std::string_view ip, std::string_view email) {
	const auto ipKey    = std::string(ip);
	const auto emailKey = normalizeEmail(email);
	const auto t        = now();

	std::lock_guard lock(mu_);
	pruneLocked(failsByIp_[ipKey], t, kIpWindow);
	pruneLocked(failsByEmail_[emailKey], t, kEmailWindow);

	TryBegin out;
	if (failsByIp_[ipKey].size() >= static_cast<size_t>(kIpFailMax)) {
		out.admit         = Admit::throttled;
		out.retryAfterSec = remainingSec(failsByIp_[ipKey], t, kIpWindow);
		return out;
	}
	if (failsByEmail_[emailKey].size() >= static_cast<size_t>(kEmailFailMax)) {
		out.admit         = Admit::throttled;
		out.retryAfterSec = remainingSec(failsByEmail_[emailKey], t, kEmailWindow);
		return out;
	}
	if (inflightIp_.contains(ipKey) || inflightEmail_.contains(emailKey)) {
		out.admit         = Admit::inflight;
		out.retryAfterSec = kInflightRetryAfterSec;
		return out;
	}
	if (argon2Live_ >= kMaxArgon2) {
		out.admit         = Admit::busy;
		out.retryAfterSec = kInflightRetryAfterSec;
		return out;
	}

	inflightIp_.insert(ipKey);
	inflightEmail_.insert(emailKey);
	++argon2Live_;
	out.admit         = Admit::ok;
	out.retryAfterSec = 0;
	out.guard         = Guard(this, ipKey, emailKey);
	return out;
}

void LoginLimiter::recordFailure(std::string_view ip, std::string_view email) {
	const auto ipKey    = std::string(ip);
	const auto emailKey = normalizeEmail(email);
	const auto t        = now();

	std::lock_guard lock(mu_);
	auto&           ipFails = failsByIp_[ipKey];
	ipFails.push_back(t);
	pruneLocked(ipFails, t, kIpWindow);

	auto& emailFails = failsByEmail_[emailKey];
	emailFails.push_back(t);
	pruneLocked(emailFails, t, kEmailWindow);
}

void LoginLimiter::recordSuccess(std::string_view email) {
	const auto emailKey = normalizeEmail(email);
	std::lock_guard lock(mu_);
	failsByEmail_.erase(emailKey);
}

void LoginLimiter::release(const std::string& ip, const std::string& email) {
	std::lock_guard lock(mu_);
	inflightIp_.erase(ip);
	inflightEmail_.erase(email);
	if (argon2Live_ > 0) {
		--argon2Live_;
	}
}

} // namespace rbk::Auth
