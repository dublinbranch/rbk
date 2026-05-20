#include <boost/test/unit_test.hpp>

#include "rbk/BoostJson/extra.h"
#include "rbk/BoostJson/math.h"

namespace bj = boost::json;

BOOST_AUTO_TEST_SUITE(boostjson_math)

BOOST_AUTO_TEST_CASE(mergeJson_inserts_new_keys)
{
	bj::object a;
	a["x"] = 1;
	bj::object b;
	b["y"] = 2;
	mergeJson(a, b, false);
	BOOST_CHECK_EQUAL(a["x"].as_int64(), 1);
	BOOST_CHECK_EQUAL(a["y"].as_int64(), 2);
}

BOOST_AUTO_TEST_CASE(mergeJson_merges_nested_objects)
{
	bj::object innerA;
	innerA["n"] = 7;
	bj::object a;
	a["o"] = innerA;

	bj::object innerB;
	innerB["m"] = 8;
	bj::object mix;
	mix["o"] = innerB;

	mergeJson(a, mix, false);
	BOOST_CHECK_EQUAL(a["o"].as_object().at("n").as_int64(), 7);
	BOOST_CHECK_EQUAL(a["o"].as_object().at("m").as_int64(), 8);
}

BOOST_AUTO_TEST_CASE(mergeJson_concatenates_arrays_when_both_sides_are_arrays)
{
	bj::array aa;
	aa.push_back(1);
	bj::object a;
	a["z"] = aa;

	bj::array ab;
	ab.push_back(2);
	bj::object mix;
	mix["z"] = ab;

	mergeJson(a, mix, false);
	BOOST_REQUIRE(a["z"].is_array());
	BOOST_CHECK_EQUAL(a["z"].as_array().size(), 2u);
}

BOOST_AUTO_TEST_CASE(subtractJson_keeps_keys_absent_from_second)
{
	bj::object first;
	first["only"] = 1;
	bj::object second;
	second["other"] = 2;
	bj::object out = subtractJson(first, second, {});
	BOOST_CHECK(out.contains("only"));
	BOOST_CHECK(!out.contains("other"));
}

BOOST_AUTO_TEST_SUITE_END()
