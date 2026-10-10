// Exercise rbk/thread/statusProbes.cpp without the app and without a DB: opens real loopback TCP
// connections (one CLOSE_WAIT, one stalled reader, one closed stalled reader, 3 never accepted), then renders every block.
// Not part of the build. Build and run: see todo/handover-statusPage-2026-10-10.md.
//   probeHarness 50 [dir]    writes dir/status.html and dir/sockets.html (default: current dir)
//   probeHarness 10000 quiet timing only (needs ulimit -n > 2 * N + 100)
#include "rbk/thread/statusProbes.h"
#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <fstream>
#include <vector>

using namespace statusProbes;

int main(int argc, char** argv) {
	const int  n     = argc > 1 ? std::atoi(argv[1]) : 50;
	const bool quiet = argc > 2 && std::string(argv[2]) == "quiet";

	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	int one = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
	sockaddr_in a{};
	a.sin_family      = AF_INET;
	a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	bind(lfd, (sockaddr*)&a, sizeof a);
	listen(lfd, 4096);
	socklen_t al = sizeof a;
	getsockname(lfd, (sockaddr*)&a, &al);

	std::vector<int> cl, sv;
	for (int i = 0; i < n; ++i) {
		int c = socket(AF_INET, SOCK_STREAM, 0);
		if (connect(c, (sockaddr*)&a, sizeof a) != 0) {
			perror("connect");
			return 1;
		}
		int s = accept(lfd, nullptr, nullptr);
		cl.push_back(c);
		sv.push_back(s);
	}
	// CLOSE_WAIT on the server side: the peer closed, we keep the fd
	close(cl[0]);
	// client that stopped reading: fill the server side buffers -> Send-Q
	{
		std::string junk(1 << 20, 'x');
		while (send(sv[1], junk.data(), junk.size(), MSG_DONTWAIT) > 0) {
		}
	}
	// stalled reader we closed: the socket loses its fd and stays in FIN_WAIT1 with its Send-Q (inode 0)
	{
		std::string junk(1 << 20, 'x');
		while (send(sv[3], junk.data(), junk.size(), MSG_DONTWAIT) > 0) {
		}
		close(sv[3]);
	}
	// unread data on our side -> Recv-Q
	send(cl[2], "hello", 5, 0);
	// accept queue: connected but never accepted
	for (int i = 0; i < 3; ++i) {
		int c = socket(AF_INET, SOCK_STREAM, 0);
		connect(c, (sockaddr*)&a, sizeof a);
	}
	epoll_create1(0);
	eventfd(0, 0);
	std::ifstream keep("/etc/hostname");
	usleep(300000);

	const auto t0   = std::chrono::steady_clock::now();
	const auto body = socketsBody(5);
	const auto ms   = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
	std::printf("sockets page: %d connections, %.1f ms\n", n, ms);
	if (quiet) {
		auto f = body.rfind("<p class=\"note\">");
		std::printf("%s", body.substr(f).c_str());
		return 0;
	}

	// The main page minus what needs the app (request table, pool threads, MySQL, cache)
	const std::string dir = argc > 2 ? argv[2] : ".";
	const auto        fds = fdCount(true);
	std::string       svc = rowN("Open fds", kInt(fds.open), "soft limit 1024 (hard 524288)") + rowT("Fd kinds", fds.kinds);
	std::string page = std::string(pageHead) + "<h1>Status · harness</h1>\n<div class=\"grid\">\n" +
	                   kvSection("service", "Service", svc) + kvSection("process", "Process", processRows(1.0, 2)) + "</div>\n" +
	                   hostSection({".", "/tmp", "/nonexistent"}, rowT("Hostname", "harness")) +
	                   networkSection(true, true, "sockets.html") + otherThreadsTable({}) + "</body></html>\n";
	std::ofstream(dir + "/status.html") << page;
	std::ofstream(dir + "/sockets.html") << std::string(pageHead) << "<h1>TCP sockets</h1>\n" << body << "</body></html>\n";
	std::printf("wrote %s/status.html and %s/sockets.html\n", dir.c_str(), dir.c_str());
	return 0;
}
