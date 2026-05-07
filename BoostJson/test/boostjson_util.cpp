#include <boost/test/unit_test.hpp>

#include "rbk/BoostJson/util.h"

namespace bj = boost::json;

BOOST_AUTO_TEST_SUITE(boostjson_util)

BOOST_AUTO_TEST_CASE(getValueFromNonsense_finds_matching_row)
{
	bj::array arr;
	bj::object row;
	row["kind"] = "alpha";
	row["payload"] = 99;
	arr.push_back(row);

	auto opt = getValueFromNonsense(arr, "kind", "alpha", "payload");
	BOOST_REQUIRE(opt.has_value());
	BOOST_CHECK_EQUAL(opt->as_int64(), 99);
}

BOOST_AUTO_TEST_CASE(getValueFromNonsense_returns_nullopt_when_key_value_mismatches)
{
	bj::array arr;
	bj::object row;
	row["kind"] = "beta";
	row["payload"] = 1;
	arr.push_back(row);

	auto opt = getValueFromNonsense(arr, "kind", "alpha", "payload");
	BOOST_CHECK(!opt.has_value());
}

BOOST_AUTO_TEST_CASE(getValueFromNonsense_returns_nullopt_for_empty_array)
{
	bj::array arr;
	auto      opt = getValueFromNonsense(arr, "a", "b", "c");
	BOOST_CHECK(!opt.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
