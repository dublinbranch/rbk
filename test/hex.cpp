#include <boost/test/unit_test.hpp>

#include "rbk/fmtExtra/includeMe.h"
#include "rbk/misc/hex.h"

#include <array>
#include <string>
#include <vector>

// The first two cases pin rbk::Hex to fmt's {:02X} / {:02x} for every byte value: the copies it
// replaced in digitalSpine wrote one or the other.

BOOST_AUTO_TEST_SUITE(hex)

BOOST_AUTO_TEST_CASE(upper_matches_fmt_for_every_byte) {
	for (unsigned v = 0; v < 256; ++v) {
		const std::array<u8, 1> b{static_cast<u8>(v)};
		BOOST_CHECK_EQUAL(rbk::Hex::upper(b), F("{:02X}", b[0]));
	}
}

BOOST_AUTO_TEST_CASE(lower_matches_fmt_for_every_byte) {
	for (unsigned v = 0; v < 256; ++v) {
		const std::array<u8, 1> b{static_cast<u8>(v)};
		BOOST_CHECK_EQUAL(rbk::Hex::lower(b), F("{:02x}", b[0]));
	}
}

BOOST_AUTO_TEST_CASE(empty) {
	BOOST_CHECK_EQUAL(rbk::Hex::upper(std::span<const u8>{}), "");
	BOOST_CHECK_EQUAL(rbk::Hex::lower(std::string_view{}), "");
	std::array<u8, 0> none{};
	BOOST_CHECK(rbk::Hex::decode("", none));
	BOOST_CHECK(!rbk::Hex::decode("00", none));
}

BOOST_AUTO_TEST_CASE(lower_of_a_raw_frame) {
	const std::string frame("\x00\x7f\x80\xff", 4);
	BOOST_CHECK_EQUAL(rbk::Hex::lower(frame), "007f80ff");
}

BOOST_AUTO_TEST_CASE(digit_edges) {
	BOOST_CHECK_EQUAL(rbk::Hex::digit('0'), 0);
	BOOST_CHECK_EQUAL(rbk::Hex::digit('9'), 9);
	BOOST_CHECK_EQUAL(rbk::Hex::digit('a'), 10);
	BOOST_CHECK_EQUAL(rbk::Hex::digit('F'), 15);
	for (const char c : std::string("/:@G`g x\0", 9)) {
		BOOST_CHECK_EQUAL(rbk::Hex::digit(c), -1);
	}
}

BOOST_AUTO_TEST_CASE(decode_either_case) {
	std::array<u8, 3> out{};
	BOOST_REQUIRE(rbk::Hex::decode("0aFf7C", out));
	BOOST_CHECK((out == std::array<u8, 3>{0x0A, 0xFF, 0x7C}));
}

BOOST_AUTO_TEST_CASE(decode_refuses) {
	std::array<u8, 2> out{};
	BOOST_CHECK(!rbk::Hex::decode("0aF", out));    // short
	BOOST_CHECK(!rbk::Hex::decode("0aFf00", out)); // long
	BOOST_CHECK(!rbk::Hex::decode("0g00", out));   // not hex
	BOOST_CHECK(!rbk::Hex::decode("0x0A", out));   // no prefix here
	BOOST_CHECK(!rbk::Hex::decode("0a:f", out));   // no separator
}

BOOST_AUTO_TEST_CASE(round_trip) {
	std::vector<u8> bytes(256);
	for (unsigned v = 0; v < 256; ++v) {
		bytes[v] = static_cast<u8>(v);
	}
	std::vector<u8> back(bytes.size());
	BOOST_REQUIRE(rbk::Hex::decode(rbk::Hex::upper(bytes), back));
	BOOST_CHECK(back == bytes);
	BOOST_REQUIRE(rbk::Hex::decode(rbk::Hex::lower(bytes), back));
	BOOST_CHECK(back == bytes);
}

BOOST_AUTO_TEST_SUITE_END()
