// Mirrors app config flow (see config.cpp): intrusivedebug.h first, parseJson, try_value_to<T>.
// BOOST_DESCRIBE_STRUCT drives value_to so BOOST_PATH_PUSH runs and BJIntrusive::composePath() maps failures.

#include "rbk/BoostJson/intrusivedebug.h"

#include "rbk/BoostJson/extra.h"

#include <boost/describe/class.hpp>
#include <boost/json/error.hpp>
#include <boost/test/unit_test.hpp>

#include <string>
#include <system_error>
#include <vector>

namespace bj = boost::json;

namespace {

struct IntrusiveLeaf {
	int value{};
};
BOOST_DESCRIBE_STRUCT(IntrusiveLeaf, (), (value));

struct IntrusiveNested {
	IntrusiveLeaf leaf{};
};
BOOST_DESCRIBE_STRUCT(IntrusiveNested, (), (leaf));

struct IntrusiveFlat {
	int         port{};
	std::string host{};
};
BOOST_DESCRIBE_STRUCT(IntrusiveFlat, (), (port, host));

void reset_intrusive_trace() {
	BJIntrusive::path.clear();
	BJIntrusive::message.clear();
	BJIntrusive::key = {};
}

} // namespace

BOOST_AUTO_TEST_SUITE(boostjson_intrusive)

BOOST_AUTO_TEST_CASE(try_value_to_described_flat_matches_config_pattern)
{
	auto r = parseJson(std::string_view(R"({"port":8080,"host":"local"})"), false);
	BOOST_REQUIRE(!r.ec);
	auto t = bj::try_value_to<IntrusiveFlat>(r.json);
	BOOST_REQUIRE(!t.has_error());
	BOOST_CHECK_EQUAL(t.value().port, 8080);
	BOOST_CHECK_EQUAL(t.value().host, "local");
}

BOOST_AUTO_TEST_CASE(try_value_to_described_nested_object_into_structure)
{
	auto r = parseJson(std::string_view(R"({"leaf":{"value":42}})"), false);
	BOOST_REQUIRE(!r.ec);
	auto t = bj::try_value_to<IntrusiveNested>(r.json);
	BOOST_REQUIRE(!t.has_error());
	BOOST_CHECK_EQUAL(t.value().leaf.value, 42);
}

BOOST_AUTO_TEST_CASE(failure_wrong_scalar_records_path_port)
{
	reset_intrusive_trace();
	auto r = parseJson(std::string_view(R"({"port":"nope","host":"x"})"), false);
	BOOST_REQUIRE(!r.ec);
	auto t = bj::try_value_to<IntrusiveFlat>(r.json);
	BOOST_REQUIRE(t.has_error());
	BOOST_CHECK_EQUAL(BJIntrusive::composePath(), "/port");
	BOOST_CHECK_EQUAL(t.error(), bj::make_error_code(bj::error::not_number));
}

BOOST_AUTO_TEST_CASE(failure_nested_leaf_records_path_leaf_value)
{
	reset_intrusive_trace();
	auto r = parseJson(std::string_view(R"({"leaf":{"value":"bad"}})"), false);
	BOOST_REQUIRE(!r.ec);
	auto t = bj::try_value_to<IntrusiveNested>(r.json);
	BOOST_REQUIRE(t.has_error());
	BOOST_CHECK_EQUAL(BJIntrusive::composePath(), "/leaf/value");
	BOOST_CHECK_EQUAL(t.error(), bj::make_error_code(bj::error::not_number));
}

BOOST_AUTO_TEST_CASE(failure_root_not_object_matches_not_object)
{
	auto r = parseJson(std::string_view(R"("only-a-string")"), false);
	BOOST_REQUIRE(!r.ec);
	auto t = bj::try_value_to<IntrusiveFlat>(r.json);
	BOOST_REQUIRE(t.has_error());
	BOOST_CHECK_EQUAL(t.error(), bj::make_error_code(bj::error::not_object));
}

BOOST_AUTO_TEST_CASE(vector_int_ok)
{
	auto r = parseJson(std::string_view(R"([1,2,3])"), false);
	BOOST_REQUIRE(!r.ec);
	auto t = bj::try_value_to<std::vector<int>>(r.json);
	BOOST_REQUIRE(!t.has_error());
	BOOST_REQUIRE_EQUAL(t.value().size(), 3u);
	BOOST_CHECK_EQUAL(t.value()[0], 1);
	BOOST_CHECK_EQUAL(t.value()[1], 2);
	BOOST_CHECK_EQUAL(t.value()[2], 3);
}

BOOST_AUTO_TEST_CASE(vector_int_root_not_array_matches_not_array)
{
	auto r = parseJson(std::string_view(R"({"not":"array"})"), false);
	BOOST_REQUIRE(!r.ec);
	auto t = bj::try_value_to<std::vector<int>>(r.json);
	BOOST_REQUIRE(t.has_error());
	BOOST_CHECK_EQUAL(t.error(), bj::make_error_code(bj::error::not_array));
}

BOOST_AUTO_TEST_CASE(vector_int_element_not_number_fails)
{
	auto r = parseJson(std::string_view(R"([1,"x",3])"), false);
	BOOST_REQUIRE(!r.ec);
	auto t = bj::try_value_to<std::vector<int>>(r.json);
	BOOST_REQUIRE(t.has_error());
	BOOST_CHECK_EQUAL(t.error(), bj::make_error_code(bj::error::not_number));
}

BOOST_AUTO_TEST_CASE(extra_json_field_matches_size_mismatch)
{
	reset_intrusive_trace();
	auto r = parseJson(std::string_view(R"({"port":1,"host":"h","extra":true})"), false);
	BOOST_REQUIRE(!r.ec);
	auto t = bj::try_value_to<IntrusiveFlat>(r.json);
	BOOST_REQUIRE(t.has_error());
	// Described object with unknown keys: converter counts matched members vs object size.
	BOOST_CHECK_EQUAL(t.error(), bj::make_error_code(bj::error::size_mismatch));
}

BOOST_AUTO_TEST_SUITE_END()
