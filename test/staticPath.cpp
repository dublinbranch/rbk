#include <boost/test/unit_test.hpp>

#include "rbk/HTTP/staticPath.h"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

BOOST_AUTO_TEST_SUITE(static_path)

BOOST_AUTO_TEST_CASE(serves_file_under_relative_root) {
	const auto got = resolveStaticFile("static", "png/a.png");
	BOOST_REQUIRE(got);
	BOOST_CHECK_EQUAL(got->lexically_normal().generic_string(), "static/png/a.png");
}

BOOST_AUTO_TEST_CASE(serves_file_under_absolute_root) {
	const fs::path root{"/var/www/static"};
	const auto     got = resolveStaticFile(root, "js/app.js");
	BOOST_REQUIRE(got);
	BOOST_CHECK_EQUAL(got->generic_string(), "/var/www/static/js/app.js");
}

BOOST_AUTO_TEST_CASE(rejects_dotdot_under_public_prefix) {
	BOOST_CHECK(!resolveStaticFile("static", "png/../../config.json"));
	BOOST_CHECK(!resolveStaticFile("static", "js/../../../etc/passwd"));
	BOOST_CHECK(!resolveStaticFile("static", "../config.json"));
	BOOST_CHECK(!resolveStaticFile("static", "png/foo/../bar.png"));
}

BOOST_AUTO_TEST_CASE(rejects_absolute_url_path) {
	BOOST_CHECK(!resolveStaticFile("static", "/etc/passwd"));
	BOOST_CHECK(!resolveStaticFile("/var/www/static", "/etc/passwd"));
}

BOOST_AUTO_TEST_CASE(rejects_backslash_and_empty) {
	BOOST_CHECK(!resolveStaticFile("static", ""));
	BOOST_CHECK(!resolveStaticFile("static", "png\\..\\..\\config.json"));
	BOOST_CHECK(!resolveStaticFile({}, "png/a.png"));
}

BOOST_AUTO_TEST_CASE(rejects_sibling_prefix) {
	BOOST_CHECK(!resolveStaticFile("static", "png/../../staticX/secret"));
}

BOOST_AUTO_TEST_SUITE_END()
