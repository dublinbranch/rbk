#pragma once

#include "rbk/BoostJson/extra.h"
#include "rbk/BoostJson/intrusivedebug.h"

#include <boost/test/unit_test.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace intrusive_test {

inline void reset_intrusive_trace() {
	BJIntrusive::path.clear();
	BJIntrusive::message.clear();
	BJIntrusive::key = {};
}

inline void print_intrusive_failure(std::string_view caseName, const boost::json::value& json,
                                    const boost::json::value& expectedShape, const boost::system::error_code& ec) {
	(void)json;
	(void)expectedShape;
	fmt::print("\n{}\n", std::string(72, '='));
	fmt::print("CASE: {}\n", caseName);
	fmt::print("{}\n", std::string(72, '-'));
	fmt::print("  try_value_to error : {} ({})\n", ec.message(), ec.value());
	fmt::print("  composePath()       : '{}'\n", BJIntrusive::composePath());
	if (!BJIntrusive::message.empty()) {
		fmt::print("  BJIntrusive::message: {}\n", BJIntrusive::message);
	}
	fmt::print("  composeMessage()    : ");
	try {
		auto msg = BJIntrusive::composeMessage(const_cast<boost::json::value*>(&json), expectedShape);
		fmt::print("{}\n", msg);
		BOOST_TEST_MESSAGE("composeMessage: " << msg);
	} catch (const std::exception& ex) {
		fmt::print("*** THREW: {}\n", ex.what());
		BOOST_TEST_MESSAGE("composeMessage threw: " << ex.what());
	}
	fmt::print("{}\n\n", std::string(72, '='));
}

inline std::filesystem::path intrusiveTestDir() {
	return std::filesystem::path(__FILE__).parent_path();
}

inline std::string loadFileText(const std::filesystem::path& path) {
	std::ifstream in(path);
	if (!in) {
		throw std::runtime_error(fmt::format("cannot open {}", path.string()));
	}
	std::ostringstream buf;
	buf << in.rdbuf();
	return buf.str();
}

} // namespace intrusive_test
