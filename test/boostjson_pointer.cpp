#include <boost/test/unit_test.hpp>

#include "rbk/BoostJson/extra.h"

#include <string>
#include <string_view>

namespace bj = boost::json;

// Covers pointer helpers (e.g. rqAtPointer / getAtPointer history) and multi-line
// JsonRes::composeErrorMsg() — historically fragile (see git: c161b0d, 7aaca27).

BOOST_AUTO_TEST_SUITE(boostjson_pointer)

BOOST_AUTO_TEST_CASE(checkStringAtPointer_true_when_value_matches)
{
	auto r = parseJson(std::string_view(R"({"id":"abc","n":1})"), false);
	BOOST_REQUIRE(!r.ec);
	BOOST_CHECK(checkStringAtPointer(r.json, "/id", "abc"));
}

BOOST_AUTO_TEST_CASE(checkStringAtPointer_false_when_value_differs)
{
	auto r = parseJson(std::string_view(R"({"id":"abc"})"), false);
	BOOST_REQUIRE(!r.ec);
	BOOST_CHECK(!checkStringAtPointer(r.json, "/id", "xyz"));
}

BOOST_AUTO_TEST_CASE(checkStringAtPointer_false_when_path_missing)
{
	auto r = parseJson(std::string_view(R"({"x":1})"), false);
	BOOST_REQUIRE(!r.ec);
	BOOST_CHECK(!checkStringAtPointer(r.json, "/missing", "a"));
}

BOOST_AUTO_TEST_CASE(getAtPointer_reads_nested_int)
{
	auto r = parseJson(std::string_view(R"({"x":{"y":42}})"), false);
	BOOST_REQUIRE(!r.ec);
	int n = getAtPointer<int>(r.json, "/x/y");
	BOOST_CHECK_EQUAL(n, 42);
}

BOOST_AUTO_TEST_CASE(composeErrorMsg_multiline_snippet_includes_context)
{
	// Invalid token on line 3 — composeErrorMsg() should still produce a useful message
	// (regression guard for multi-line snippet logic).
	const std::string raw = "{\n\"a\": 1,\n  not_json\n}";
	auto              res   = parseJson(std::string_view(raw), false);
	BOOST_REQUIRE(res.ec.failed());
	std::string msg = res.composeErrorMsg();
	BOOST_CHECK(msg.find("Invalid") != std::string::npos);
	BOOST_CHECK(msg.find("line") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
