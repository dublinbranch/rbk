#include <boost/test/unit_test.hpp>

#include "rbk/rateLimit/rateLimit.h"

// rbk::Bucket and rbk::RateState on a clock the test moves (ns).

namespace {
constexpr i64 kS = 1'000'000'000;
using Why        = rbk::RateState::Why;
} // namespace

BOOST_AUTO_TEST_SUITE(rate_limit)

BOOST_AUTO_TEST_CASE(bucket_starts_full_refills_and_caps) {
	rbk::Bucket b;
	// 60 a minute = 1 a second, cap 3
	BOOST_CHECK(b.take(100 * kS, 60, 3));
	BOOST_CHECK(b.take(100 * kS, 60, 3));
	BOOST_CHECK(b.take(100 * kS, 60, 3));
	BOOST_CHECK(!b.take(100 * kS, 60, 3));
	BOOST_CHECK_CLOSE(b.secondsUntil(100 * kS, 60, 3, 1), 1.0, 1e-6);
	BOOST_CHECK(b.take(101 * kS, 60, 3));
	// a long pause fills it to the cap, not more
	BOOST_CHECK_CLOSE(b.peek(1000 * kS, 60, 3), 3.0, 1e-9);
	// a clock that goes back adds nothing and takes nothing
	BOOST_CHECK(!b.take(50 * kS, 60, 3));
}

BOOST_AUTO_TEST_CASE(bucket_debt_has_a_floor) {
	rbk::Bucket b;
	b.debit(0, 6000, 6000, 100000, -6000);
	BOOST_CHECK_CLOSE(b.peek(0, 6000, 6000), -6000.0, 1e-9);
	BOOST_CHECK(!b.positive(0, 6000, 6000));
	// 100 ms a second: 60 s back to 0
	BOOST_CHECK(!b.positive(59 * kS, 6000, 6000));
	BOOST_CHECK(b.positive(61 * kS, 6000, 6000));
}

BOOST_AUTO_TEST_CASE(requests_refused_with_retry_after) {
	rbk::RateState s;
	const rbk::Limits l{.reqPerMin = 60, .reqBurst = 2, .busyMsPerMin = 0, .inFlight = 0};
	Why                why = Why::none;
	BOOST_CHECK_EQUAL(s.admit(10 * kS, l, why), 0u);
	BOOST_CHECK_EQUAL(s.admit(10 * kS, l, why), 0u);
	BOOST_CHECK_EQUAL(s.admit(10 * kS, l, why), 1u);
	BOOST_CHECK(why == Why::requests);
	BOOST_CHECK_EQUAL(s.admit(11 * kS, l, why), 0u);
	BOOST_CHECK(why == Why::none);
	// skipRequests: the request bucket is not used
	BOOST_CHECK_EQUAL(s.admit(11 * kS, l, why, true), 0u);
}

BOOST_AUTO_TEST_CASE(rate_zero_is_no_limit_and_burst_zero_is_one) {
	rbk::RateState s;
	Why            why = Why::none;
	for (int i = 0; i < 1000; ++i) {
		BOOST_REQUIRE_EQUAL(s.admit(0, rbk::Limits{}, why), 0u);
	}
	rbk::RateState    t;
	const rbk::Limits oneMin{.reqPerMin = 1, .reqBurst = 0, .busyMsPerMin = 0, .inFlight = 0};
	BOOST_CHECK_EQUAL(t.admit(0, oneMin, why), 0u); // cap 1, not 0: not refused for good
	BOOST_CHECK_EQUAL(t.admit(0, oneMin, why), 60u);
}

BOOST_AUTO_TEST_CASE(busy_refused_until_it_refills_and_takes_no_token) {
	rbk::RateState s;
	// 1 request a minute: the one token tells whether a busy refusal took it
	const rbk::Limits l{.reqPerMin = 1, .reqBurst = 1, .busyMsPerMin = 6000, .inFlight = 0};
	Why               why = Why::none;
	BOOST_CHECK_EQUAL(s.admit(0, l, why), 0u);
	s.debit(0, l, 600000); // a 10 min request: the debt stops at one minute
	BOOST_CHECK_CLOSE(s.view(0, l).busyLevel, -6000.0, 1e-9);
	BOOST_CHECK_EQUAL(s.admit(10 * kS, l, why), 51u); // 100 ms a second back from -5000
	BOOST_CHECK(why == Why::busy);
	BOOST_CHECK_EQUAL(s.admit(60 * kS, l, why), 1u); // at 0, not above: still refused
	BOOST_CHECK(why == Why::busy);
	// the token of the request bucket is back since 60 s, and the refusals did not take it
	BOOST_CHECK_EQUAL(s.admit(61 * kS, l, why), 0u);
	// no limit on busy: no debit
	rbk::RateState free;
	free.debit(0, rbk::Limits{}, 1e9);
	BOOST_CHECK_EQUAL(free.admit(0, rbk::Limits{}, why), 0u);
}

BOOST_AUTO_TEST_CASE(in_flight_and_idle) {
	rbk::RateState    s;
	const rbk::Limits l{.reqPerMin = 60, .reqBurst = 2, .busyMsPerMin = 6000, .inFlight = 2};
	BOOST_CHECK(s.enter(l.inFlight));
	BOOST_CHECK(s.enter(l.inFlight));
	BOOST_CHECK(!s.enter(l.inFlight));
	BOOST_CHECK_EQUAL(s.view(0, l).inFlight, 2u);
	s.leave();
	BOOST_CHECK(s.enter(l.inFlight));
	BOOST_CHECK(s.enter(0)); // 0 = no limit, still counted
	BOOST_CHECK_EQUAL(s.view(0, l).inFlight, 3u);
	s.leave();
	s.leave();
	BOOST_CHECK(!s.idle(0, l));
	s.leave();
	BOOST_CHECK(s.idle(0, l)); // never used: both full
	Why why = Why::none;
	BOOST_CHECK_EQUAL(s.admit(0, l, why), 0u);
	BOOST_CHECK(!s.idle(0, l));
	BOOST_CHECK(s.idle(2 * kS, l));
	s.noteRefused(1234);
	BOOST_CHECK_EQUAL(s.view(0, l).refused, 1u);
	BOOST_CHECK_EQUAL(s.view(0, l).lastRefused, 1234);
}

BOOST_AUTO_TEST_SUITE_END()
