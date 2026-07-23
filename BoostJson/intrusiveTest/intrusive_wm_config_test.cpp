// Decode PPPLC config_WM.json with described structs mirroring PPPLC/misc/describeconfig.h.
//
// Run:
//   cmake --build build --target rbk_tests
//   ./build/test/rbk_tests --run_test=boostjson_intrusive_wm

#include "rbk/BoostJson/intrusivedebug.h"

#include "intrusive_test_util.h"
#include "wm_config_types.h"

#include "rbk/BoostJson/extra.h"

#include <boost/describe/class.hpp>
#include <boost/json/conversion.hpp>
#include <boost/json/value_from.hpp>
#include <boost/test/unit_test.hpp>

#include <fmt/format.h>

namespace bj = boost::json;

static_assert(boost::describe::has_describe_members<wm_config::Config>::value);
static_assert(bj::is_described_class<wm_config::Config>::value);
using intrusive_test::loadFileText;
using intrusive_test::print_intrusive_failure;
using intrusive_test::reset_intrusive_trace;

namespace {

std::filesystem::path configWmPath() {
	return intrusive_test::intrusiveTestDir() / "config_WM.json";
}

bj::value loadConfigWmJson() {
	auto text = loadFileText(configWmPath());
	auto res  = parseJson(text, false);
	if (res.ec) {
		throw std::runtime_error(fmt::format("parse error in {}: {}", configWmPath().string(), res.ec.message()));
	}
	return res.json;
}

void decodeConfigWm(std::string_view caseName) {
	reset_intrusive_trace();
	auto json = loadConfigWmJson();
	auto t    = bj::try_value_to<wm_config::Config>(json);
	if (!t.has_error()) {
		fmt::print("\nCASE: {} — decode OK\n\n", caseName);
		return;
	}

	print_intrusive_failure(caseName, json, bj::value_from(wm_config::defaultConfig()), t.error());
}

} // namespace

BOOST_AUTO_TEST_SUITE(boostjson_intrusive_wm)

BOOST_AUTO_TEST_CASE(config_wm_json_file_exists)
{
	BOOST_REQUIRE(std::filesystem::exists(configWmPath()));
}

BOOST_AUTO_TEST_CASE(config_wm_parses)
{
	BOOST_REQUIRE_NO_THROW(loadConfigWmJson());
}

BOOST_AUTO_TEST_CASE(config_wm_decode_like_ppplc)
{
	// Same flow as PPPLC config.cpp: parse file, try_value_to<Config>, print intrusive diagnostics on failure.
	reset_intrusive_trace();
	auto json = loadConfigWmJson();
	auto t    = bj::try_value_to<wm_config::Config>(json);
	BOOST_REQUIRE(t.has_error());
	BOOST_CHECK_EQUAL(t.error(), bj::make_error_code(bj::error::size_mismatch));

	auto msg = BJIntrusive::composeMessage(&json, bj::value_from(wm_config::defaultConfig()));
	BOOST_CHECK_EQUAL(msg, "Extra JSON key: /svai");

	decodeConfigWm("PPPLC config_WM.json full decode");
}

BOOST_AUTO_TEST_SUITE_END()
