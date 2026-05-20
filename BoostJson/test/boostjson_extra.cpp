#include <boost/test/unit_test.hpp>

#include "rbk/BoostJson/extra.h"

#include <string>
#include <string_view>

namespace bj = boost::json;

BOOST_AUTO_TEST_SUITE(boostjson_extra)

BOOST_AUTO_TEST_CASE(escape_json_escapes_quotes_and_backslash)
{
	BOOST_CHECK_EQUAL(escape_json(std::string(R"(a"b\c)")), std::string(R"(a\"b\\c)"));
}

BOOST_AUTO_TEST_CASE(pretty_print_object_contains_keys)
{
	auto r = parseJson(std::string_view(R"({"z":1})"), false);
	BOOST_REQUIRE(!r.ec);
	std::string s = pretty_print(r.json);
	BOOST_CHECK(s.find('{') != std::string::npos);
	BOOST_CHECK(s.find('z') != std::string::npos);
}

BOOST_AUTO_TEST_CASE(join_serializes_json_array)
{
	bj::array a;
	a.push_back("x");
	a.push_back("y");
	std::string s = join(a);
	BOOST_CHECK(s.find('x') != std::string::npos);
	BOOST_CHECK(s.find('y') != std::string::npos);
}

BOOST_AUTO_TEST_CASE(asString_reads_object_field)
{
	auto r = parseJson(std::string_view(R"({"msg":"hi"})"), false);
	BOOST_REQUIRE(r.json.is_object());
	std::string_view sv = asString(r.json.as_object(), "msg");
	BOOST_CHECK_EQUAL(sv, "hi");
}

BOOST_AUTO_TEST_SUITE_END()
