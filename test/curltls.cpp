// TLS verification of the minCurl v2 calls (minCurl/todo/curl-opt-v2.md), against a local `openssl s_server`
// with a throwaway self-signed certificate for "localhost". Skipped when the openssl tool is missing.

#include <boost/test/unit_test.hpp>

#include "rbk/minCurl/mincurl.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <spawn.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <thread>

extern char** environ;

namespace {

namespace fs = std::filesystem;

int freePort() {
	const int   fd = ::socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in a{};
	a.sin_family      = AF_INET;
	a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	a.sin_port        = 0;
	socklen_t len     = sizeof(a);
	int       port    = 0;
	if (::bind(fd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == 0 &&
	    ::getsockname(fd, reinterpret_cast<sockaddr*>(&a), &len) == 0) {
		port = ntohs(a.sin_port);
	}
	::close(fd);
	return port;
}

bool canConnect(int port) {
	const int   fd = ::socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in a{};
	a.sin_family      = AF_INET;
	a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	a.sin_port        = htons(static_cast<uint16_t>(port));
	const bool ok     = ::connect(fd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) == 0;
	::close(fd);
	return ok;
}

/** openssl s_server -www on 127.0.0.1, certificate CN/SAN = localhost. */
struct TlsServer {
	fs::path dir;
	fs::path cert;
	int      port = 0;
	pid_t    pid  = 0;
	bool     up   = false;

	TlsServer() {
		if (std::system("openssl version > /dev/null 2>&1") != 0) {
			return;
		}
		std::string tmpl = (fs::temp_directory_path() / "rbk-curltls-XXXXXX").string();
		if (!::mkdtemp(tmpl.data())) {
			return;
		}
		dir                 = tmpl;
		cert                = dir / "cert.pem";
		const auto key      = dir / "key.pem";
		const auto makeCert = std::format("openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes "
		                                  "-keyout '{}' -out '{}' -days 1 -subj /CN=localhost "
		                                  "-addext subjectAltName=DNS:localhost > /dev/null 2>&1",
		                                  key.string(), cert.string());
		if (std::system(makeCert.c_str()) != 0) {
			return;
		}
		port                               = freePort();
		const auto                 accept  = std::format("127.0.0.1:{}", port);
		const auto                 certArg = cert.string();
		const auto                 keyArg  = key.string();
		const char*                argv[]  = {"openssl", "s_server", "-quiet", "-www", "-accept", accept.c_str(),
		                                      "-cert", certArg.c_str(), "-key", keyArg.c_str(), nullptr};
		posix_spawn_file_actions_t fa;
		posix_spawn_file_actions_init(&fa);
		posix_spawn_file_actions_addopen(&fa, 0, "/dev/null", O_RDONLY, 0);
		posix_spawn_file_actions_addopen(&fa, 1, "/dev/null", O_WRONLY, 0);
		posix_spawn_file_actions_addopen(&fa, 2, "/dev/null", O_WRONLY, 0);
		const int rc = posix_spawnp(&pid, "openssl", &fa, nullptr, const_cast<char* const*>(argv), environ);
		posix_spawn_file_actions_destroy(&fa);
		if (rc != 0) {
			pid = 0;
			return;
		}
		for (int i = 0; i < 100 && !up; ++i) {
			up = canConnect(port);
			if (!up) {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		}
	}

	~TlsServer() {
		if (pid > 0) {
			::kill(pid, SIGTERM);
			::waitpid(pid, nullptr, 0);
		}
		if (!dir.empty()) {
			std::error_code ec;
			fs::remove_all(dir, ec);
		}
	}

	std::string url(const char* host) const {
		return std::format("https://{}:{}/", host, port);
	}
};

#define NEED_SERVER(srv)                                                        \
	if (!(srv).up) {                                                            \
		BOOST_TEST_MESSAGE("openssl s_server not available, TLS test skipped"); \
		return;                                                                 \
	}

} // namespace

BOOST_AUTO_TEST_SUITE(curl_tls)

BOOST_AUTO_TEST_CASE(v2_defaults_verify) {
	const CurlOpt opt;
	BOOST_CHECK(opt.verifyTls);
	BOOST_CHECK(opt.caFile.empty());
	BOOST_CHECK_EQUAL(opt.timeoutSec, 60);
	BOOST_CHECK(!opt.okNeeds2xx);
	BOOST_CHECK(opt.curl == nullptr);
}

BOOST_AUTO_TEST_CASE(v2_rejects_an_untrusted_certificate) {
	TlsServer srv;
	NEED_SERVER(srv);
	const auto get = urlGetV2(srv.url("localhost"), {.timeoutSec = 5, .quiet = true});
	BOOST_CHECK(!get.ok);
	BOOST_CHECK_EQUAL(get.errorCode, CURLE_PEER_FAILED_VERIFICATION);
	BOOST_CHECK(!get.errorMsg.isEmpty());

	// The B2 case: a JSON POST must not reach a server it cannot verify.
	const auto post = urlPostV2(srv.url("localhost"), "{}",
	                            {.timeoutSec = 5, .headers = {"Content-Type: application/json"}, .quiet = true});
	BOOST_CHECK(!post.ok);
	BOOST_CHECK_EQUAL(post.errorCode, CURLE_PEER_FAILED_VERIFICATION);
}

BOOST_AUTO_TEST_CASE(v2_accepts_a_pinned_ca) {
	TlsServer srv;
	NEED_SERVER(srv);
	const auto res = urlGetV2(srv.url("localhost"), {.caFile = srv.cert.string(), .timeoutSec = 5, .quiet = true});
	BOOST_CHECK_MESSAGE(res.ok, res.errorMsg.toStdString());
	BOOST_CHECK_EQUAL(res.httpCode, 200);
	BOOST_CHECK_EQUAL(res.ip.toStdString(), "127.0.0.1");
}

BOOST_AUTO_TEST_CASE(v2_checks_the_host_name) {
	TlsServer srv;
	NEED_SERVER(srv);
	// Trusted chain, but the certificate names "localhost", not 127.0.0.1.
	const auto res = urlGetV2(srv.url("127.0.0.1"), {.caFile = srv.cert.string(), .timeoutSec = 5, .quiet = true});
	BOOST_CHECK(!res.ok);
	BOOST_CHECK_EQUAL(res.errorCode, CURLE_PEER_FAILED_VERIFICATION);
}

BOOST_AUTO_TEST_CASE(v2_verify_off_and_reused_handle) {
	TlsServer srv;
	NEED_SERVER(srv);
	CurlKeeper keeper;
	for (int i = 0; i < 2; ++i) { // second call on the same handle: nothing left over from the first
		const auto res = urlGetV2(srv.url("localhost"), {.verifyTls = false, .timeoutSec = 5, .quiet = true, .curl = &keeper});
		BOOST_CHECK_MESSAGE(res.ok, res.errorMsg.toStdString());
		BOOST_CHECK_EQUAL(res.httpCode, 200);
	}
	// verify back on, same handle: the earlier verify-off call must not stick
	const auto strict = urlGetV2(srv.url("localhost"), {.timeoutSec = 5, .quiet = true, .curl = &keeper});
	BOOST_CHECK_EQUAL(strict.errorCode, CURLE_PEER_FAILED_VERIFICATION);
}

BOOST_AUTO_TEST_CASE(v1_keeps_verification_off) {
	TlsServer srv;
	NEED_SERVER(srv);
	const auto res = urlGetContent2(QByteArray::fromStdString(srv.url("localhost")), true);
	BOOST_CHECK_MESSAGE(res.ok, res.errorMsg.toStdString());
	BOOST_CHECK_EQUAL(res.httpCode, 200);
	BOOST_CHECK(res.redirectUrl.isEmpty()); // v1 without a handle never filled it
}

BOOST_AUTO_TEST_SUITE_END()
