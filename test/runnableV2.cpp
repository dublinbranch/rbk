#include <boost/test/unit_test.hpp>

#include "rbk/misc/QDebugHandler.h"
#include "rbk/misc/runnableV2.h"

using rbk::RunnableV2;

BOOST_AUTO_TEST_SUITE(runnable_v2)

BOOST_AUTO_TEST_CASE(first_call_runs) {
	RunnableV2 r;
	r.fakeNowSec = 1000;
	auto res     = r("a.cpp:10", 100);
	BOOST_CHECK(res.run);
	BOOST_CHECK_EQUAL(res.suppressed, 0u);
}

BOOST_AUTO_TEST_CASE(inside_window_is_blocked_and_counted) {
	RunnableV2 r;
	r.fakeNowSec = 1000;
	BOOST_REQUIRE(r("a.cpp:10", 100).run);
	r.fakeNowSec = 1010;
	BOOST_CHECK(!r("a.cpp:10", 100).run);
	r.fakeNowSec = 1099;
	BOOST_CHECK(!r("a.cpp:10", 100).run);
	// Window ends at last run + coolDown: 1100 runs and reports the 2 blocked calls.
	r.fakeNowSec = 1100;
	auto res     = r("a.cpp:10", 100);
	BOOST_CHECK(res.run);
	BOOST_CHECK_EQUAL(res.suppressed, 2u);
	BOOST_CHECK_EQUAL(res.firstSuppressedSec, 1010);
}

BOOST_AUTO_TEST_CASE(counter_resets_after_run) {
	RunnableV2 r;
	r.fakeNowSec = 1000;
	BOOST_REQUIRE(r("k", 100).run);
	r.fakeNowSec = 1050;
	BOOST_REQUIRE(!r("k", 100).run);
	r.fakeNowSec = 1200;
	BOOST_REQUIRE(r("k", 100).run);
	// New window starts at 1200, no blocked calls in it yet.
	r.fakeNowSec = 1250;
	BOOST_CHECK(!r("k", 100).run);
	r.fakeNowSec = 1300;
	auto res     = r("k", 100);
	BOOST_CHECK(res.run);
	BOOST_CHECK_EQUAL(res.suppressed, 1u);
	BOOST_CHECK_EQUAL(res.firstSuppressedSec, 1250);
}

BOOST_AUTO_TEST_CASE(zero_cooldown_always_runs) {
	RunnableV2 r;
	r.fakeNowSec = 1000;
	for (int i = 0; i < 5; ++i) {
		auto res = r("k", 0);
		BOOST_CHECK(res.run);
		BOOST_CHECK_EQUAL(res.suppressed, 0u);
	}
}

BOOST_AUTO_TEST_CASE(keys_are_independent) {
	RunnableV2 r;
	r.fakeNowSec = 1000;
	BOOST_REQUIRE(r("a", 100).run);
	r.fakeNowSec = 1001;
	BOOST_CHECK(r("b", 100).run);
	r.fakeNowSec = 1002;
	BOOST_CHECK(!r("a", 100).run);
}

BOOST_AUTO_TEST_CASE(clock_going_back_runs) {
	RunnableV2 r;
	r.fakeNowSec = 1000;
	BOOST_REQUIRE(r("k", 100).run);
	r.fakeNowSec = 500;
	BOOST_CHECK(r("k", 100).run);
	// The new last run is 500.
	r.fakeNowSec = 550;
	BOOST_CHECK(!r("k", 100).run);
}

BOOST_AUTO_TEST_CASE(system_clock_default) {
	RunnableV2 r;
	BOOST_CHECK(r("k", 3600).run);
	BOOST_CHECK(!r("k", 3600).run);
}

BOOST_AUTO_TEST_SUITE_END()

// Key choice of the warning mail in generalMsgHandler.
BOOST_AUTO_TEST_SUITE(warning_mail_key)

BOOST_AUTO_TEST_CASE(file_line_when_qt_gives_file) {
	QMessageLogContext ctx("src/a.cpp", 42, "void f()", "default");
	BOOST_CHECK_EQUAL(warningMailKey(ctx, "url http://x at 12:00"), "src/a.cpp:42");
}

BOOST_AUTO_TEST_CASE(text_ignored_when_qt_gives_file) {
	QMessageLogContext ctx("src/a.cpp", 42, "void f()", "default");
	BOOST_CHECK_EQUAL(warningMailKey(ctx, "first"), warningMailKey(ctx, "second"));
}

BOOST_AUTO_TEST_CASE(text_prefix_when_no_file) {
	QMessageLogContext ctx(nullptr, 0, nullptr, "default");
	BOOST_CHECK_EQUAL(warningMailKey(ctx, "QSqlDatabase: no driver"), "QSqlDatabase: no driver");
	BOOST_CHECK_NE(warningMailKey(ctx, "first"), warningMailKey(ctx, "second"));
	BOOST_CHECK_EQUAL(warningMailKey(ctx, QString(500, 'x')).size(), 120u);
}

BOOST_AUTO_TEST_CASE(same_line_blocked_other_line_runs) {
	RunnableV2         r;
	QMessageLogContext a("src/a.cpp", 10, "void f()", "default");
	QMessageLogContext b("src/a.cpp", 11, "void f()", "default");
	r.fakeNowSec = 1000;
	BOOST_REQUIRE(r(warningMailKey(a, "sync failed"), 3600).run);
	BOOST_CHECK(!r(warningMailKey(a, "sync failed again"), 3600).run);
	BOOST_CHECK(r(warningMailKey(b, "heartbeat failed"), 3600).run);
}

BOOST_AUTO_TEST_SUITE_END()
