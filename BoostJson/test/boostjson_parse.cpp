#include <boost/test/unit_test.hpp>

#include "rbk/BoostJson/extra.h"
#include "rbk/QStacker/exceptionv2.h"

#include <string>
#include <string_view>

BOOST_AUTO_TEST_SUITE(boostjson_parse)

BOOST_AUTO_TEST_CASE(parse_invalid_nonfinite_numbers_throw_exceptionv2)
{
	const char* bad = R"({"type":"SmartPlug","decoder":"shellyV1","serial":"08927264DB70","power":inf,"minPower":inf,"maxPower":inf,"voltage":230,"energy":0,"temperature":11.730860})";
	BOOST_CHECK_THROW(parseJson(std::string_view(bad), true), ExceptionV2);
}

BOOST_AUTO_TEST_CASE(parse_invalid_inf_exception_message_is_non_empty)
{
	const char* bad = R"({"x":inf})";
	try {
		parseJson(std::string_view(bad), true);
		BOOST_FAIL("parse unexpectedly succeeded");
	} catch (const ExceptionV2& e) {
		BOOST_REQUIRE_NE(e.what(), nullptr);
		BOOST_CHECK(!std::string(e.what()).empty());
	}
}

BOOST_AUTO_TEST_CASE(parse_empty_without_throw_yields_null_value)
{
	auto r = parseJson(std::string_view(), false);
	BOOST_CHECK(r.json.is_null());
}

BOOST_AUTO_TEST_CASE(parse_empty_with_throw)
{
	BOOST_CHECK_THROW(parseJson(std::string_view(), true), ExceptionV2);
}

BOOST_AUTO_TEST_CASE(parse_valid_object)
{
	auto r = parseJson(std::string_view(R"({"k":42})"), false);
	BOOST_CHECK(!r.ec);
	BOOST_REQUIRE(r.json.is_object());
	BOOST_CHECK_EQUAL(r.json.as_object().at("k").as_int64(), 42);
}

BOOST_AUTO_TEST_CASE(parse_accepts_comments_and_trailing_comma)
{
	auto r = parseJson(std::string_view(R"({"a":1,} // tail)"), false);
	BOOST_CHECK(!r.ec);
	BOOST_REQUIRE(r.json.is_object());
	BOOST_CHECK_EQUAL(r.json.as_object().at("a").as_int64(), 1);
}

BOOST_AUTO_TEST_CASE(parse_syntax_error_without_throw_sets_error_code)
{
	auto r = parseJson(std::string_view("{"), false);
	BOOST_CHECK(r.ec.failed());
}

BOOST_AUTO_TEST_SUITE_END()
