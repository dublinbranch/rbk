#include <boost/test/unit_test.hpp>

#include "rbk/HTTP/PMFCGI.h"
#include "rbk/HTTP/Payload.h"
#include "rbk/HTTP/authAdv.h"
#include "rbk/HTTP/authHandler.h"
#include "rbk/HTTP/beast.h"
#include "rbk/HTTP/beastConfig.h"
#include "rbk/HTTP/router.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

// The request-log hooks: what rbk records about a request (lastCheck, routeKey, secretPath, method)
// and the two BeastConf hooks, the last ones through a real loopback connection (handle_request).

using rbk::Auth::CheckResult;
using rbk::Auth::LastCheck;

// The program's main DB, which rbk/thread/tmonitoring.cpp expects (pulled in by the status page of
// router.cpp). No DB here, as in demo/shutdown/main.cpp.
class DB;
DB* mainDB = nullptr;

namespace {

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;
using namespace std::chrono_literals;

int handled = 0;
void handler(PMFCGI&, Payload& payload) {
	++handled;
	payload.html = "plain";
}
// Passes its guard, then refuses on its own.
void refusingHandler(PMFCGI& status, Payload& payload) {
	rbk::Auth::denyRoute(status, payload, 403);
}
void throwingHandler(PMFCGI&, Payload&) {
	throw std::runtime_error("handler test");
}

enum TestPriv : int { Alpha = 0, Beta = 1 };

// Sets the rbk::Auth and rbk::AuthAdv callbacks for one case, restores them after.
struct AuthFixture {
	rbk::Auth::Conf    savedBase = rbk::Auth::conf();
	rbk::AuthAdv::Conf savedAdv  = rbk::AuthAdv::conf();
	bool               logged    = true;
	bool               root      = false;
	int                level     = 0;
	rbk::AuthAdv::Bits bits{0, 0};

	AuthFixture() {
		handled = 0;
		rbk::Auth::resetLastCheck();
		auto& b             = rbk::Auth::conf();
		b.isLogged          = [this] { return logged; };
		b.hasLevel          = [this](int minLevel) { return level >= minLevel; };
		b.auditBlockedRoute = nullptr;
		auto& a             = rbk::AuthAdv::conf();
		a.isRoot            = [this] { return root; };
		a.bitsOfCurrentUser = [this]() -> const rbk::AuthAdv::Bits& { return bits; };
	}
	~AuthFixture() {
		rbk::Auth::conf()    = savedBase;
		rbk::AuthAdv::conf() = savedAdv;
		rbk::Auth::resetLastCheck();
	}
	AuthFixture(const AuthFixture&)            = delete;
	AuthFixture& operator=(const AuthFixture&) = delete;
};

void requireCheck(int priv, int level, u8 op, CheckResult result, bool byHandler = false) {
	const LastCheck& lc = rbk::Auth::lastCheck();
	BOOST_CHECK_EQUAL(lc.priv, priv);
	BOOST_CHECK_EQUAL(lc.level, level);
	BOOST_CHECK_EQUAL(static_cast<int>(lc.op), static_cast<int>(op));
	BOOST_CHECK(lc.result == result);
	BOOST_CHECK_EQUAL(lc.byHandler, byHandler);
}

constexpr u8 kRead  = static_cast<u8>(rbk::AuthAdv::Op::read);
constexpr u8 kWrite = static_cast<u8>(rbk::AuthAdv::Op::write);

std::filesystem::path uniqueTempDir(std::string_view name) {
	auto dir = std::filesystem::temp_directory_path() / (std::string(name) + "-" + std::to_string(::getpid()));
	std::filesystem::create_directories(dir);
	return dir;
}

// getDefaultRouting() hands out raw news; the BeastConf of a test owns them.
void deleteRouting(BeastConf& conf) {
	for (auto& [key, route] : conf.routing) {
		delete route;
	}
	conf.routing.clear();
}

// What the hooks saw, copied on the server thread.
struct Seen {
	std::string method;
	std::string routeKey;
	bool        secretPath = false;
	unsigned    statusCode = 0;
	size_t      respBytes  = 0;
	bool        file       = false;
	LastCheck   check;
};

// One HTTP connection to a server running handle_request on its own thread.
struct Loopback {
	BeastConf             conf;
	net::io_context       serverIoc{1};
	net::io_context       clientIoc;
	beast::tcp_stream     client{clientIoc};
	beast::flat_buffer    buf;
	std::thread           server;
	std::filesystem::path logDir = uniqueTempDir("rbk-request-hooks-log");

	std::mutex        mu;
	int               starts = 0;
	std::vector<Seen> done;
	std::atomic<bool> throwInStart{false};
	std::atomic<bool> throwInDone{false};

	// configure: extra BeastConf settings, applied before the server runs.
	explicit Loopback(const std::function<void(BeastConf&)>& configure = {}) {
		conf.logFolder = logDir.string();
		conf.routingSimple.insert({"plain", handler});
		conf.routingSimple.insert({"guarded", rbk::Auth::guard<1, handler>});
		conf.routingSimple.insert({"boom", throwingHandler});
		conf.routingSimple.insert({"", handler});
		if (configure) {
			configure(conf);
		}
		conf.onRequestStart = [this] {
			std::lock_guard lock(mu);
			++starts;
			if (throwInStart) {
				throw std::runtime_error("start hook test");
			}
		};
		conf.onRequestDone = [this](const PMFCGI& status, const Payload& payload, const RequestDone& d) {
			std::lock_guard lock(mu);
			done.push_back({status.method, status.routeKey, status.secretPath, payload.statusCode, d.respBytes, d.file,
			                rbk::Auth::lastCheck()});
			if (throwInDone) {
				throw std::runtime_error("done hook test");
			}
		};

		net::basic_socket_acceptor<tcp, net::io_context::executor_type> acc(
		    serverIoc.get_executor(), tcp::endpoint(net::ip::make_address("127.0.0.1"), 0));
		client.socket().connect(acc.local_endpoint()); // completes from the backlog, before accept()
		serveHttpConnection(acc.accept(), &conf, 30s);
		server = std::thread([this] { serverIoc.run(); });
	}
	~Loopback() {
		beast::error_code ec;
		client.socket().shutdown(tcp::socket::shutdown_both, ec);
		client.close();
		serverIoc.stop();
		server.join();
		std::filesystem::remove_all(logDir, ec);
	}
	Loopback(const Loopback&)            = delete;
	Loopback& operator=(const Loopback&) = delete;

	// One client operation, at most 10 s.
	template <class Start>
	beast::error_code run(Start start) {
		beast::error_code ec = net::error::would_block;
		client.expires_after(10s);
		start([&](beast::error_code e, std::size_t) { ec = e; });
		clientIoc.restart();
		clientIoc.run();
		return ec;
	}

	http::response<http::string_body> ask(std::string_view method, std::string_view target) {
		const auto raw = std::string(method).append(" ").append(target).append(" HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
		BOOST_REQUIRE(!run([&](auto h) { net::async_write(client, net::buffer(raw), h); }));
		http::response_parser<http::string_body> parser;
		const auto                               ec = run([&](auto h) { http::async_read(client, buf, parser, h); });
		BOOST_REQUIRE_MESSAGE(!ec, "no answer: " << ec.message());
		return parser.release();
	}

	// The hooks run before the answer is written, so they are done when it is read.
	int startCount() {
		std::lock_guard lock(mu);
		return starts;
	}
	std::vector<Seen> doneSeen() {
		std::lock_guard lock(mu);
		return done;
	}
	std::string fileInLog(const char* name) {
		std::ifstream in(logDir / name);
		return {std::istreambuf_iterator<char>(in), {}};
	}
	std::string exceptionLog() {
		return fileInLog("stdException.log");
	}
};

} // namespace

BOOST_AUTO_TEST_SUITE(request_hooks)

BOOST_FIXTURE_TEST_CASE(reset_clears_the_last_check, AuthFixture) {
	rbk::Auth::lastCheck() = LastCheck{.priv = 3, .level = 2, .op = kWrite, .result = CheckResult::root, .byHandler = true};
	rbk::Auth::resetLastCheck();
	requireCheck(-1, -1, 0, CheckResult::none);
}

BOOST_FIXTURE_TEST_CASE(level_guard_records_pass_401_403, AuthFixture) {
	PMFCGI  status;
	Payload payload;

	level = 2;
	rbk::Auth::guard<2, handler>(status, payload);
	requireCheck(-1, 2, 0, CheckResult::pass);
	BOOST_CHECK_EQUAL(handled, 1);

	level = 1;
	rbk::Auth::guard<2, handler>(status, payload);
	requireCheck(-1, 2, 0, CheckResult::deny403);

	logged = false;
	rbk::Auth::guard<2, handler>(status, payload);
	requireCheck(-1, 2, 0, CheckResult::deny401);
	BOOST_CHECK_EQUAL(handled, 1);
}

BOOST_FIXTURE_TEST_CASE(priv_guard_records_pass_root_401_403, AuthFixture) {
	PMFCGI  status;
	Payload payload;

	bits[Beta] = kRead;
	rbk::AuthAdv::canRead<Beta, handler>(status, payload);
	requireCheck(Beta, -1, kRead, CheckResult::pass);

	rbk::AuthAdv::canWrite<Beta, handler>(status, payload);
	requireCheck(Beta, -1, kWrite, CheckResult::deny403);

	root = true;
	rbk::AuthAdv::canWrite<Beta, handler>(status, payload);
	requireCheck(Beta, -1, kWrite, CheckResult::root);

	logged = false;
	rbk::AuthAdv::canRead<Alpha, handler>(status, payload);
	requireCheck(Alpha, -1, kRead, CheckResult::deny401);
	BOOST_CHECK_EQUAL(handled, 2);
}

BOOST_FIXTURE_TEST_CASE(need_write_replaces_the_route_check, AuthFixture) {
	PMFCGI  status;
	Payload payload;

	bits[Alpha] = kRead;
	rbk::AuthAdv::canRead<Alpha, handler>(status, payload);
	requireCheck(Alpha, -1, kRead, CheckResult::pass);
	BOOST_CHECK(!rbk::AuthAdv::needWrite<Alpha>(status, payload));
	requireCheck(Alpha, -1, kWrite, CheckResult::deny403);

	bits[Alpha] = kRead | kWrite;
	BOOST_CHECK(rbk::AuthAdv::needWrite<Alpha>(status, payload));
	requireCheck(Alpha, -1, kWrite, CheckResult::pass);
}

BOOST_FIXTURE_TEST_CASE(handler_refusal_after_a_pass_is_marked, AuthFixture) {
	PMFCGI  status;
	Payload payload;

	bits[Alpha] = kRead;
	rbk::AuthAdv::canRead<Alpha, refusingHandler>(status, payload);
	requireCheck(Alpha, -1, kRead, CheckResult::deny403, true);

	level = 1;
	rbk::Auth::guard<1, refusingHandler>(status, payload);
	requireCheck(-1, 1, 0, CheckResult::deny403, true);

	// a project guard that is not rbk's: only its deny shows
	rbk::Auth::resetLastCheck();
	rbk::Auth::denyRoute(status, payload, 401);
	requireCheck(-1, -1, 0, CheckResult::deny401, false);
}

BOOST_FIXTURE_TEST_CASE(can_matches_check, AuthFixture) {
	bits[Alpha] = kRead;
	BOOST_CHECK(rbk::AuthAdv::can(Alpha, rbk::AuthAdv::Op::read));
	BOOST_CHECK(!rbk::AuthAdv::can(Alpha, rbk::AuthAdv::Op::write));
	BOOST_CHECK(!rbk::AuthAdv::can(7, rbk::AuthAdv::Op::read)); // past the bits
	root = true;
	BOOST_CHECK(rbk::AuthAdv::check(7, rbk::AuthAdv::Op::write) == CheckResult::root);
	// check() leaves lastCheck alone: only the guards record
	requireCheck(-1, -1, 0, CheckResult::none);
}

BOOST_AUTO_TEST_CASE(router_sets_the_route_key) {
	BeastConf conf;
	conf.routingSimple.insert({"api/x", handler});
	conf.routing = getDefaultRouting();

	const auto root = uniqueTempDir("rbk-request-hooks-static");
	std::filesystem::create_directories(root / "js");
	std::ofstream(root / "js" / "app.js") << "1;";
	conf.staticFile = root;

	auto run = [&](const std::string& path) {
		PMFCGI  status;
		Payload payload;
		status.conf = &conf;
		status.path = "/" + path;
		status.url  = Url(QString::fromStdString("http://localhost/" + path));
		Router router;
		router.immediate(status, &conf, payload);
		return status.routeKey;
	};

	BOOST_CHECK_EQUAL(run("api/x"), "api/x");
	BOOST_CHECK_EQUAL(run("echo"), "echo");
	BOOST_CHECK_EQUAL(run(jemallocPagePath), "<rbk>"); // a matched token page, its path not kept
	BOOST_CHECK_EQUAL(run("js/app.js"), "<static>");
	BOOST_CHECK_EQUAL(run("no/such/route"), "<unknown>");

	deleteRouting(conf);
	std::filesystem::remove_all(root);
}

BOOST_AUTO_TEST_CASE(secret_rbk_tokens) {
	BOOST_CHECK(hasSecretRbkToken(statusPagePath));
	BOOST_CHECK(hasSecretRbkToken(statusSocketsPath));
	BOOST_CHECK(hasSecretRbkToken(jemallocPagePath));
	BOOST_CHECK(hasSecretRbkToken(jemallocStatsPath));
	BOOST_CHECK(hasSecretRbkToken(std::string("/") + statusPagePath + "/sockets/")); // a near miss
	BOOST_CHECK(hasSecretRbkToken(std::string("//") + jemallocPagePath));             // a doubled slash
	const std::string status(statusPagePath);
	BOOST_CHECK(hasSecretRbkToken(status.substr(0, 20)));                       // cut short
	BOOST_CHECK(hasSecretRbkToken(status.substr(0, status.size() - 1) + "x"));  // typo at the end
	BOOST_CHECK(hasSecretRbkToken("Q" + status.substr(1)));                     // typo at the start
	std::string upper = status;
	std::transform(upper.begin(), upper.end(), upper.begin(), [](char c) { return static_cast<char>(std::toupper(c)); });
	BOOST_CHECK(hasSecretRbkToken(upper)); // another case
	BOOST_CHECK(!hasSecretRbkToken("echo"));
	BOOST_CHECK(!hasSecretRbkToken("/api/leaf/led/rgb?x=1"));
	BOOST_CHECK(!hasSecretRbkToken(""));
}

BOOST_AUTO_TEST_CASE(hooks_run_once_and_see_the_request) {
	Loopback lb;
	const auto res = lb.ask("GET", "/plain");
	BOOST_CHECK_EQUAL(res.result_int(), 200U);
	BOOST_CHECK_EQUAL(res.body(), "plain");

	BOOST_CHECK_EQUAL(lb.startCount(), 1);
	const auto seen = lb.doneSeen();
	BOOST_REQUIRE_EQUAL(seen.size(), 1U);
	BOOST_CHECK_EQUAL(seen[0].method, "GET");
	BOOST_CHECK_EQUAL(seen[0].routeKey, "plain");
	BOOST_CHECK(!seen[0].secretPath);
	BOOST_CHECK_EQUAL(seen[0].statusCode, 200U);
	BOOST_CHECK_EQUAL(seen[0].respBytes, 5U);
	BOOST_CHECK(!seen[0].file);
	BOOST_CHECK(seen[0].check.result == CheckResult::none); // no guard on this route

	lb.ask("POST", "/plain");
	BOOST_CHECK_EQUAL(lb.startCount(), 2);
	BOOST_REQUIRE_EQUAL(lb.doneSeen().size(), 2U);
	BOOST_CHECK_EQUAL(lb.doneSeen()[1].method, "POST");
}

BOOST_FIXTURE_TEST_CASE(done_hook_sees_the_guard_of_the_request, AuthFixture) {
	logged = false;
	Loopback   lb;
	const auto res = lb.ask("GET", "/guarded");
	BOOST_CHECK_EQUAL(res.result_int(), 401U);
	const auto seen = lb.doneSeen();
	BOOST_REQUIRE_EQUAL(seen.size(), 1U);
	BOOST_CHECK_EQUAL(seen[0].check.level, 1);
	BOOST_CHECK(seen[0].check.result == CheckResult::deny401);

	// the next request on the same worker thread starts clean
	lb.ask("GET", "/plain");
	BOOST_REQUIRE_EQUAL(lb.doneSeen().size(), 2U);
	BOOST_CHECK(lb.doneSeen()[1].check.result == CheckResult::none);
	BOOST_CHECK_EQUAL(lb.doneSeen()[1].check.level, -1);
}

BOOST_AUTO_TEST_CASE(options_skips_both_hooks) {
	Loopback   lb;
	const auto res = lb.ask("OPTIONS", "/plain");
	BOOST_CHECK_EQUAL(res.result_int(), 200U);
	BOOST_CHECK_EQUAL(lb.startCount(), 0);
	BOOST_CHECK(lb.doneSeen().empty());
}

BOOST_AUTO_TEST_CASE(unknown_method_is_not_kept_as_sent) {
	Loopback lb;
	lb.ask("FOOBARBAZ", "/plain");
	const auto seen = lb.doneSeen();
	BOOST_REQUIRE_EQUAL(seen.size(), 1U);
	BOOST_CHECK_EQUAL(seen[0].method, "<unknown>");
}

BOOST_AUTO_TEST_CASE(secret_token_is_flagged_on_a_miss) {
	Loopback   lb;
	const auto res = lb.ask("GET", std::string("/") + statusPagePath + "/nope/");
	BOOST_CHECK_EQUAL(res.result_int(), 400U);
	const auto seen = lb.doneSeen();
	BOOST_REQUIRE_EQUAL(seen.size(), 1U);
	BOOST_CHECK(seen[0].secretPath);
	BOOST_CHECK_EQUAL(seen[0].routeKey, "<unknown>");
}

BOOST_AUTO_TEST_CASE(throwing_done_hook_keeps_the_answer) {
	Loopback lb;
	lb.throwInDone = true;
	const auto res = lb.ask("GET", "/plain");
	BOOST_CHECK_EQUAL(res.result_int(), 200U);
	BOOST_CHECK_EQUAL(res.body(), "plain");
	BOOST_CHECK_EQUAL(lb.doneSeen().size(), 1U);
	BOOST_CHECK(lb.exceptionLog().find("onRequestDone failed: done hook test") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(failed_start_hook_skips_the_done_hook) {
	Loopback lb;
	lb.throwInStart = true;
	const auto res  = lb.ask("GET", "/plain");
	BOOST_CHECK_EQUAL(res.result_int(), 200U);
	BOOST_CHECK_EQUAL(res.body(), "plain");
	BOOST_CHECK_EQUAL(lb.startCount(), 1);
	BOOST_CHECK(lb.doneSeen().empty());
	BOOST_CHECK(lb.exceptionLog().find("onRequestStart failed: start hook test") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(percent_encoded_token_is_flagged) {
	Loopback    lb;
	std::string encoded = std::string("/%5A") + (statusPagePath + 1); // Z sent as %5A
	lb.ask("GET", encoded);
	lb.ask("GET", std::string("/plain?x=") + "%5A" + (jemallocPagePath + 1)); // in the query
	const auto seen = lb.doneSeen();
	BOOST_REQUIRE_EQUAL(seen.size(), 2U);
	BOOST_CHECK(seen[0].secretPath);
	BOOST_CHECK(seen[1].secretPath);
	BOOST_CHECK_EQUAL(seen[1].routeKey, "plain");
}

BOOST_AUTO_TEST_CASE(throwing_handler_is_seen_once_with_its_500) {
	Loopback   lb;
	const auto res = lb.ask("GET", "/boom");
	BOOST_CHECK_EQUAL(res.result_int(), 500U);
	const auto seen = lb.doneSeen();
	BOOST_REQUIRE_EQUAL(seen.size(), 1U);
	BOOST_CHECK_EQUAL(seen[0].statusCode, 500U);
	BOOST_CHECK_EQUAL(seen[0].routeKey, "boom");
}

BOOST_AUTO_TEST_CASE(site_root_has_its_own_route_key) {
	Loopback lb;
	lb.ask("GET", "/");
	const auto seen = lb.doneSeen();
	BOOST_REQUIRE_EQUAL(seen.size(), 1U);
	BOOST_CHECK_EQUAL(seen[0].routeKey, "/");
}

// rbk's own access log: a missing list used to throw after the hook had seen the answer (the
// client then got a canned 500); a token page is written as <rbk>, with no answer.
BOOST_AUTO_TEST_CASE(access_log_with_a_missing_list_and_a_secret_path) {
	Loopback lb([](BeastConf& c) {
		c.logRequest   = true;
		c.logResponse  = true;
		c.logWhitelist = std::vector<std::string>{"plain"};
		c.logBlacklist.reset();
	});
	const auto res = lb.ask("GET", "/plain");
	BOOST_CHECK_EQUAL(res.result_int(), 200U);
	BOOST_CHECK_EQUAL(res.body(), "plain");
	lb.ask("GET", std::string("/") + statusPagePath + "/nope/");

	const auto seen = lb.doneSeen();
	BOOST_REQUIRE_EQUAL(seen.size(), 2U);
	BOOST_CHECK_EQUAL(seen[0].statusCode, 200U);
	const auto log = lb.fileInLog("access.log");
	BOOST_CHECK(log.find("/plain 200") != std::string::npos);
	BOOST_CHECK(log.find("<rbk> 400") != std::string::npos);
	BOOST_CHECK(!hasSecretRbkToken(log));
}

BOOST_AUTO_TEST_SUITE_END()
