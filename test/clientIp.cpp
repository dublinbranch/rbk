#include <boost/test/unit_test.hpp>

#include "rbk/HTTP/clientIp.h"

#include <string>
#include <vector>

using rbk::Http::isTrustedProxy;
using rbk::Http::normalizeAddress;
using rbk::Http::parseAddress;
using rbk::Http::rightmostForwardedFor;

namespace {

boost::asio::ip::address addr(const char* text) {
	return boost::asio::ip::make_address(text);
}

} // namespace

BOOST_AUTO_TEST_SUITE(client_ip)

BOOST_AUTO_TEST_CASE(parseAddress_keeps_real_addresses) {
	BOOST_CHECK_EQUAL(parseAddress("203.0.113.7").value_or("-"), "203.0.113.7");
	BOOST_CHECK_EQUAL(parseAddress("2001:db8::1").value_or("-"), "2001:db8::1");
	BOOST_CHECK_EQUAL(parseAddress("::ffff:203.0.113.7").value_or("-"), "::ffff:203.0.113.7");
}

BOOST_AUTO_TEST_CASE(parseAddress_rejects_text) {
	BOOST_CHECK(!parseAddress("").has_value());
	BOOST_CHECK(!parseAddress("example.com").has_value());
	BOOST_CHECK(!parseAddress("1.2.3.4.5").has_value());
	BOOST_CHECK(!parseAddress("1.2.3.256").has_value());
	BOOST_CHECK(!parseAddress("' OR 1=1 --").has_value());
	BOOST_CHECK(!parseAddress("1.2.3.4' --").has_value());
}

// The inet_aton spellings. QHostAddress accepts every one of these and folds the first four
// to 127.0.0.1, which is why this file uses Boost.Asio. See HTTP/docs/clientIp.md.
BOOST_AUTO_TEST_CASE(parseAddress_rejects_the_loose_inet_aton_forms) {
	BOOST_CHECK(!parseAddress("2130706433").has_value());
	BOOST_CHECK(!parseAddress("0x7f.0.0.1").has_value());
	BOOST_CHECK(!parseAddress("127.000.000.001").has_value());
	BOOST_CHECK(!parseAddress("1.2.3").has_value());
	BOOST_CHECK(!parseAddress(" 1.2.3.4").has_value());
	BOOST_CHECK(!parseAddress("1.2.3.4 ").has_value());
}

BOOST_AUTO_TEST_CASE(normalizeAddress_strips_the_v4_mapped_prefix) {
	BOOST_CHECK_EQUAL(normalizeAddress("::ffff:203.0.113.7").value_or("-"), "203.0.113.7");
	BOOST_CHECK_EQUAL(normalizeAddress("203.0.113.7").value_or("-"), "203.0.113.7");
	BOOST_CHECK_EQUAL(normalizeAddress("2001:db8::1").value_or("-"), "2001:db8::1");
}

BOOST_AUTO_TEST_CASE(normalizeAddress_rejects_a_mapped_prefix_on_garbage) {
	BOOST_CHECK(!normalizeAddress("::ffff:' OR 1=1 --").has_value());
	BOOST_CHECK(!normalizeAddress("").has_value());
	BOOST_CHECK(!normalizeAddress("example.com").has_value());
}

// nginx $proxy_add_x_forwarded_for APPENDS the peer it saw, so the last element is the only
// one the client could not write.
BOOST_AUTO_TEST_CASE(rightmostForwardedFor_picks_the_last_element) {
	BOOST_CHECK_EQUAL(rightmostForwardedFor("10.0.0.9, 203.0.113.7").value_or("-"), "203.0.113.7");
	BOOST_CHECK_EQUAL(rightmostForwardedFor("203.0.113.7").value_or("-"), "203.0.113.7");
	BOOST_CHECK_EQUAL(rightmostForwardedFor("a, b, 203.0.113.7").value_or("-"), "203.0.113.7");
}

BOOST_AUTO_TEST_CASE(rightmostForwardedFor_trims_spaces_and_tabs) {
	BOOST_CHECK_EQUAL(rightmostForwardedFor("10.0.0.9,\t 203.0.113.7 ").value_or("-"), "203.0.113.7");
}

BOOST_AUTO_TEST_CASE(rightmostForwardedFor_rejects_an_injection_payload) {
	BOOST_CHECK(!rightmostForwardedFor("' UNION SELECT id,email,hashed,'x',1,1 FROM account --").has_value());
	BOOST_CHECK(!rightmostForwardedFor("203.0.113.7, '; UPDATE account SET x=1; -- ").has_value());
	BOOST_CHECK(!rightmostForwardedFor("").has_value());
	BOOST_CHECK(!rightmostForwardedFor("  ").has_value());
	BOOST_CHECK(!rightmostForwardedFor("10.0.0.9,").has_value());
}

BOOST_AUTO_TEST_CASE(isTrustedProxy_loopback_needs_no_config) {
	BOOST_CHECK(isTrustedProxy(addr("127.0.0.1"), std::nullopt));
	BOOST_CHECK(isTrustedProxy(addr("::1"), std::nullopt));
}

BOOST_AUTO_TEST_CASE(isTrustedProxy_denies_by_default) {
	BOOST_CHECK(!isTrustedProxy(addr("203.0.113.7"), std::nullopt));
	BOOST_CHECK(!isTrustedProxy(addr("10.0.0.9"), std::nullopt));
}

BOOST_AUTO_TEST_CASE(isTrustedProxy_matches_a_listed_address) {
	const std::vector<std::string> list = {"10.0.0.2", "2001:db8::2"};
	BOOST_CHECK(isTrustedProxy(addr("10.0.0.2"), list));
	BOOST_CHECK(isTrustedProxy(addr("2001:db8::2"), list));
	BOOST_CHECK(!isTrustedProxy(addr("10.0.0.3"), list));
}

// A typo in the config must not widen the match.
BOOST_AUTO_TEST_CASE(isTrustedProxy_ignores_unparsable_entries) {
	const std::vector<std::string> list = {"not an address", "10.0.0.0/8", ""};
	BOOST_CHECK(!isTrustedProxy(addr("10.4.5.6"), list));
}

BOOST_AUTO_TEST_SUITE_END()
