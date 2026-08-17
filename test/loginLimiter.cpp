#include <boost/test/unit_test.hpp>

#include "rbk/HTTP/loginLimiter.h"

#include <chrono>
#include <stdexcept>
#include <string>

using namespace std::chrono;
using rbk::Auth::LoginLimiter;
using Admit = LoginLimiter::Admit;

BOOST_AUTO_TEST_SUITE(login_limiter)

BOOST_AUTO_TEST_CASE(second_same_ip_is_inflight) {
	LoginLimiter lim;
	auto         first = lim.tryBegin("10.0.0.1", "a@example.com");
	BOOST_REQUIRE(first.admit == Admit::ok);
	auto second = lim.tryBegin("10.0.0.1", "b@example.com");
	BOOST_CHECK(second.admit == Admit::inflight);
	BOOST_CHECK_EQUAL(second.retryAfterSec, LoginLimiter::kInflightRetryAfterSec);
}

BOOST_AUTO_TEST_CASE(second_same_email_is_inflight) {
	LoginLimiter lim;
	auto         first = lim.tryBegin("10.0.0.1", "Alice@Example.com");
	BOOST_REQUIRE(first.admit == Admit::ok);
	auto second = lim.tryBegin("10.0.0.2", "alice@example.com");
	BOOST_CHECK(second.admit == Admit::inflight);
}

BOOST_AUTO_TEST_CASE(third_pair_is_busy) {
	LoginLimiter lim;
	auto         a = lim.tryBegin("10.0.0.1", "a@example.com");
	auto         b = lim.tryBegin("10.0.0.2", "b@example.com");
	BOOST_REQUIRE(a.admit == Admit::ok);
	BOOST_REQUIRE(b.admit == Admit::ok);
	auto c = lim.tryBegin("10.0.0.3", "c@example.com");
	BOOST_CHECK(c.admit == Admit::busy);
}

BOOST_AUTO_TEST_CASE(guard_releases_on_exception) {
	LoginLimiter lim;
	try {
		auto slot = lim.tryBegin("10.0.0.1", "a@example.com");
		BOOST_REQUIRE(slot.admit == Admit::ok);
		throw std::runtime_error("boom");
	} catch (const std::runtime_error&) {
	}
	auto again = lim.tryBegin("10.0.0.1", "a@example.com");
	BOOST_CHECK(again.admit == Admit::ok);
}

BOOST_AUTO_TEST_CASE(ip_failure_window) {
	auto         now = steady_clock::now();
	LoginLimiter lim([&now] {
		return now;
	});
	for (int i = 0; i < LoginLimiter::kIpFailMax; ++i) {
		lim.recordFailure("9.9.9.9", "n" + std::to_string(i) + "@example.com");
	}
	auto blocked = lim.tryBegin("9.9.9.9", "other@example.com");
	BOOST_CHECK(blocked.admit == Admit::throttled);
	BOOST_CHECK(blocked.retryAfterSec >= 1);

	now += LoginLimiter::kIpWindow + seconds(1);
	auto after = lim.tryBegin("9.9.9.9", "other@example.com");
	BOOST_CHECK(after.admit == Admit::ok);
}

BOOST_AUTO_TEST_CASE(email_failure_window) {
	auto         now = steady_clock::now();
	LoginLimiter lim([&now] {
		return now;
	});
	for (int i = 0; i < LoginLimiter::kEmailFailMax; ++i) {
		lim.recordFailure("10.0.0." + std::to_string(i), "same@example.com");
	}
	auto blocked = lim.tryBegin("1.2.3.4", "same@example.com");
	BOOST_CHECK(blocked.admit == Admit::throttled);

	now += LoginLimiter::kEmailWindow + seconds(1);
	auto after = lim.tryBegin("1.2.3.4", "same@example.com");
	BOOST_CHECK(after.admit == Admit::ok);
}

BOOST_AUTO_TEST_CASE(success_clears_email_window) {
	LoginLimiter lim;
	for (int i = 0; i < LoginLimiter::kEmailFailMax; ++i) {
		lim.recordFailure("10.0.0." + std::to_string(i), "same@example.com");
	}
	BOOST_CHECK(lim.tryBegin("1.2.3.4", "same@example.com").admit == Admit::throttled);
	lim.recordSuccess("same@example.com");
	auto after = lim.tryBegin("1.2.3.4", "same@example.com");
	BOOST_CHECK(after.admit == Admit::ok);
}

BOOST_AUTO_TEST_SUITE_END()
