// rbk::Shutdown (thread/shutdown.h). install() runs once per process and cannot be undone, so
// every case runs in a forked child. Signals are sent with kill(getpid(), sig), as from outside.
#include "rbk/misc/QDebugHandler.h"
#include "rbk/thread/shutdown.h"

#include <QCoreApplication>
#include <QProcess>

#include <boost/test/unit_test.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <cstdlib>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std::chrono_literals;
namespace Sd = rbk::Shutdown;

namespace {

using Clock = std::chrono::steady_clock;

// steady_clock is CLOCK_MONOTONIC on Linux: the same clock in the parent and the child.
long long nowNs() {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
}

// For the child: write a line to stderr (the pipe to the parent), without stdio buffers.
void say(const std::string& line) {
	auto                  l = line + "\n";
	[[maybe_unused]] auto w = write(STDERR_FILENO, l.data(), l.size());
}

// For the child: mark "now" under key, so the parent can time what happens after it.
void mark(const char* key) {
	say(std::string(key) + "=" + std::to_string(nowNs()));
}

// For the child: fail with a message.
int fail(const std::string& why) {
	say("FAIL " + why);
	return 1;
}

struct ChildResult {
	int         status   = 0;
	bool        timedOut = false;
	long long   endNs    = 0; // when the parent saw the child end
	std::string err;          // everything the child wrote to stderr

	bool exited(int code) const { return WIFEXITED(status) && WEXITSTATUS(status) == code; }
	bool killedBy(int sig) const { return WIFSIGNALED(status) && WTERMSIG(status) == sig; }

	// Seconds from the mark key to the end of the child; -1 if the mark is missing.
	double since(const char* key) const {
		auto k   = std::string(key) + "=";
		auto pos = err.find(k);
		if (pos == std::string::npos) {
			return -1;
		}
		return static_cast<double>(endNs - std::stoll(err.substr(pos + k.size()))) / 1e9;
	}
};

// Run body in a forked child with stderr piped to the parent. The child starts with SIGINT,
// SIGTERM and SIGALRM at SIG_DFL and nothing blocked, and always ends with _exit.
ChildResult runChild(const std::function<int()>& body, std::chrono::seconds timeout = 15s) {
	int fds[2];
	BOOST_REQUIRE(pipe(fds) == 0);
	fflush(nullptr);
	pid_t pid = fork();
	BOOST_REQUIRE(pid >= 0);
	if (pid == 0) {
		close(fds[0]);
		dup2(fds[1], STDERR_FILENO);
		close(fds[1]);
		for (int sig : {SIGINT, SIGTERM, SIGALRM}) {
			signal(sig, SIG_DFL);
		}
		sigset_t none;
		sigemptyset(&none);
		sigprocmask(SIG_SETMASK, &none, nullptr);
		int code = 2;
		try {
			code = body();
		} catch (const std::exception& e) {
			say(std::string("FAIL exception: ") + e.what());
		}
		_exit(code);
	}

	close(fds[1]);
	fcntl(fds[0], F_SETFL, O_NONBLOCK);
	ChildResult r;
	auto        end = Clock::now() + timeout;
	char        buf[4096];
	for (;;) {
		ssize_t n;
		while ((n = read(fds[0], buf, sizeof(buf))) > 0) {
			r.err.append(buf, n);
		}
		pid_t got = waitpid(pid, &r.status, WNOHANG);
		if (got == pid) {
			r.endNs = nowNs();
			break;
		}
		if (Clock::now() > end) {
			kill(pid, SIGKILL);
			waitpid(pid, &r.status, 0);
			r.timedOut = true;
			break;
		}
		std::this_thread::sleep_for(1ms);
	}
	ssize_t n;
	while ((n = read(fds[0], buf, sizeof(buf))) > 0) {
		r.err.append(buf, n);
	}
	close(fds[0]);
	BOOST_TEST_MESSAGE("child stderr:\n"
	                   << r.err);
	return r;
}

void stuckThread() {
	std::thread([] {
		Sd::Loop guard{"stuck"};
		std::this_thread::sleep_for(1h);
	}).detach();
}

} // namespace

BOOST_AUTO_TEST_SUITE(shutdown)

// T1: SIGTERM wakes a sleeper at once, runs the callbacks once each in reverse order.
BOOST_AUTO_TEST_CASE(sigterm_wakes_sleepers_and_runs_callbacks) {
	auto r = runChild([] {
		if (!Sd::install(5s)) {
			return fail("install");
		}
		std::atomic<int> order[2] = {0, 0};
		std::atomic<int> calls{0};
		auto             a = Sd::onStop([&] { order[calls++] = 1; });
		auto             b = Sd::onStop([&] { order[calls++] = 2; });

		std::atomic<long long> wokeNs{0};
		std::atomic<bool>      slept{true};
		std::thread            sleeper([&] {
			slept  = Sd::sleepFor(1h);
			wokeNs = nowNs();
		});
		std::this_thread::sleep_for(50ms);
		long long sentNs = nowNs();
		kill(getpid(), SIGTERM);
		sleeper.join();
		if (slept) {
			return fail("sleepFor returned true");
		}
		if (wokeNs - sentNs > 100'000'000) {
			return fail("sleepFor woke after " + std::to_string((wokeNs - sentNs) / 1'000'000) + " ms");
		}
		for (int i = 0; i < 100 && calls < 2; ++i) {
			std::this_thread::sleep_for(10ms);
		}
		std::this_thread::sleep_for(50ms);
		if (calls != 2 || order[0] != 2 || order[1] != 1) {
			return fail("callbacks: calls=" + std::to_string(calls) + " order=" + std::to_string(order[0]) + std::to_string(order[1]));
		}
		if (Sd::reason() != Sd::Reason::sigterm) {
			return fail("reason");
		}
		return 0;
	});
	BOOST_TEST(r.exited(0), r.err);
}

// T2: request() twice: callbacks once, reason request, no forced exit.
BOOST_AUTO_TEST_CASE(request_twice) {
	auto r = runChild([] {
		if (!Sd::install(5s)) {
			return fail("install");
		}
		std::atomic<int> calls{0};
		auto             reg = Sd::onStop([&] { ++calls; });
		Sd::request();
		Sd::request();
		for (int i = 0; i < 100 && Sd::running(); ++i) {
			std::this_thread::sleep_for(10ms);
		}
		Sd::request();
		std::this_thread::sleep_for(300ms); // a forced exit would kill us here
		if (Sd::running() || calls != 1) {
			return fail("calls=" + std::to_string(calls));
		}
		if (Sd::reason() != Sd::Reason::request) {
			return fail("reason");
		}
		return 0;
	});
	BOOST_TEST(r.exited(0), r.err);
}

// T3: onStop after shutdown started runs fn at once, on the caller's thread.
BOOST_AUTO_TEST_CASE(late_on_stop_runs_at_once) {
	auto r = runChild([] {
		if (!Sd::install(5s)) {
			return fail("install");
		}
		kill(getpid(), SIGTERM);
		for (int i = 0; i < 100 && Sd::running(); ++i) {
			std::this_thread::sleep_for(10ms);
		}
		if (Sd::running()) {
			return fail("still running");
		}
		std::thread::id ranOn;
		auto            reg = Sd::onStop([&] { ranOn = std::this_thread::get_id(); });
		if (ranOn != std::this_thread::get_id()) {
			return fail("fn did not run on the caller before onStop returned");
		}
		return 0;
	});
	BOOST_TEST(r.exited(0), r.err);
}

// T4: Wakeup is sticky, and times out without a notify.
BOOST_AUTO_TEST_CASE(wakeup_sticky_and_timeout) {
	auto r = runChild([] {
		Sd::Wakeup w;
		w.notify();
		auto t0 = Clock::now();
		if (w.waitFor(1h) != Sd::Wakeup::Result::woken || Clock::now() - t0 > 50ms) {
			return fail("sticky notify");
		}
		t0 = Clock::now();
		if (w.waitFor(10ms) != Sd::Wakeup::Result::timeout) {
			return fail("expected timeout");
		}
		if (Clock::now() - t0 < 10ms) {
			return fail("timeout too early");
		}
		return 0;
	});
	BOOST_TEST(r.exited(0), r.err);
}

// T5: no lost wakeup. A producer notifies in a loop, a consumer waits until stopping.
BOOST_AUTO_TEST_CASE(no_lost_wakeup_stress) {
	int failures = 0;
	for (int round = 0; round < 200; ++round) {
		auto r = runChild([] {
			if (!Sd::install(5s)) {
				return fail("install");
			}
			Sd::Wakeup             w;
			std::atomic<long long> doneNs{0};
			std::thread            producer([&] {
				while (Sd::running()) {
					w.notify();
				}
			});
			std::thread            consumer([&] {
				while (w.waitFor(1h) != Sd::Wakeup::Result::stopping) {
				}
				doneNs = nowNs();
			});
			std::this_thread::sleep_for(20ms);
			long long sentNs = nowNs();
			kill(getpid(), SIGTERM);
			consumer.join();
			producer.join();
			if (doneNs - sentNs > 100'000'000) {
				return fail("consumer returned after " + std::to_string((doneNs - sentNs) / 1'000'000) + " ms");
			}
			return 0;
		});
		if (!r.exited(0)) {
			++failures;
			BOOST_TEST_MESSAGE("round " << round << ": " << r.err);
		}
	}
	BOOST_TEST(failures == 0);
}

// T6: a stuck loop: forced exit by SIGTERM at the deadline, naming the loop.
BOOST_AUTO_TEST_CASE(forced_exit_at_deadline) {
	auto r = runChild([] {
		if (!Sd::install(1s)) {
			return fail("install");
		}
		stuckThread();
		std::this_thread::sleep_for(50ms);
		mark("sent");
		kill(getpid(), SIGTERM);
		std::this_thread::sleep_for(1h);
		return 0;
	});
	BOOST_TEST(r.killedBy(SIGTERM), r.err);
	auto t = r.since("sent");
	BOOST_TEST((t >= 1.0 && t <= 1.5), "t=" << t);
	BOOST_TEST(r.err.find("forced after") != std::string::npos);
	BOOST_TEST(r.err.find("stuck") != std::string::npos);
}

// T7: a second signal forces the exit at once.
BOOST_AUTO_TEST_CASE(second_signal_forces_exit) {
	auto r = runChild([] {
		if (!Sd::install(30s)) {
			return fail("install");
		}
		stuckThread();
		std::this_thread::sleep_for(50ms);
		kill(getpid(), SIGTERM);
		std::this_thread::sleep_for(100ms);
		mark("sent2");
		kill(getpid(), SIGTERM);
		std::this_thread::sleep_for(1h);
		return 0;
	});
	BOOST_TEST(r.killedBy(SIGTERM), r.err);
	auto t = r.since("sent2");
	BOOST_TEST((t >= 0 && t <= 0.3), "t=" << t);
}

// T8: a thread started before install() gets the signal handling too: no ordering rule.
BOOST_AUTO_TEST_CASE(thread_started_before_install) {
	auto r = runChild([] {
		std::atomic<bool> slept{true};
		std::thread       sleeper([&] { slept = Sd::sleepFor(1h); });
		std::this_thread::sleep_for(20ms);
		if (!Sd::install(5s)) {
			return fail("install");
		}
		kill(getpid(), SIGTERM);
		sleeper.join();
		if (slept || Sd::reason() != Sd::Reason::sigterm) {
			return fail("not woken by SIGTERM");
		}
		return 0;
	});
	BOOST_TEST(r.exited(0), r.err);
}

// T9: other code that replaces the SIGTERM handler: the watcher warns within about 2 s.
BOOST_AUTO_TEST_CASE(warns_when_handler_replaced) {
	auto r = runChild([] {
		if (!Sd::install(5s)) {
			return fail("install");
		}
		signal(SIGTERM, SIG_IGN);
		std::this_thread::sleep_for(2500ms);
		return 0;
	});
	BOOST_TEST(r.exited(0), r.err);
	BOOST_TEST(r.err.find("SIGTERM handler was replaced") != std::string::npos);
}

// T10: children keep the default SIGINT/SIGTERM behaviour: QProcess, and popen (posix_spawn).
BOOST_AUTO_TEST_CASE(children_keep_default_signals) {
	auto r = runChild([] {
		int              argc   = 1;
		char             name[] = "rbk_tests";
		char*            argv[] = {name, nullptr};
		QCoreApplication app(argc, argv);
		if (!Sd::install(5s)) {
			return fail("install");
		}
		auto parse = [](const QString& line) {
			return line.section(':', 1).trimmed().toULongLong(nullptr, 16);
		};

		QProcess p;
		p.start("grep", {"SigBlk", "/proc/self/status"});
		p.waitForFinished(5000);
		auto qprocMask = parse(QString::fromUtf8(p.readAllStandardOutput()));

		char  line[256] = {};
		FILE* f         = popen("grep SigBlk /proc/self/status", "r");
		if (!f || !fgets(line, sizeof(line), f)) {
			return fail("popen");
		}
		pclose(f);
		auto popenMask = parse(QString::fromUtf8(line));

		say("masks " + std::to_string(qprocMask) + " " + std::to_string(popenMask));
		if ((qprocMask & 0x4002) != 0 || (popenMask & 0x4002) != 0) {
			return fail("a child has SIGINT/SIGTERM blocked");
		}

		// And a child really stops on SIGTERM.
		QProcess sleeper;
		sleeper.start("sleep", {"600"});
		if (!sleeper.waitForStarted(5000)) {
			return fail("sleep did not start");
		}
		kill(static_cast<pid_t>(sleeper.processId()), SIGTERM);
		if (!sleeper.waitForFinished(2000) || sleeper.exitStatus() != QProcess::CrashExit) {
			return fail("child did not stop on SIGTERM");
		}
		return 0;
	});
	BOOST_TEST(r.exited(0), r.err);
}

// T11: a callback that blocks forever: the SIGALRM backstop kills the process.
BOOST_AUTO_TEST_CASE(alarm_backstop) {
	auto r = runChild([] {
		if (!Sd::install(1s)) {
			return fail("install");
		}
		auto reg = Sd::onStop([] {
			for (;;) {
				pause();
			}
		});
		mark("sent");
		kill(getpid(), SIGTERM);
		std::this_thread::sleep_for(1h);
		return 0;
	});
	BOOST_TEST(r.killedBy(SIGALRM), r.err);
	auto t = r.since("sent");
	BOOST_TEST((t >= 5.5 && t <= 7), "t=" << t);
}

// T12: a thread blocks on a full stdout while it holds the log lock. The watcher must not wait
// for it: the callbacks run, and the forced exit comes at the deadline, not from SIGALRM.
BOOST_AUTO_TEST_CASE(watcher_does_not_wait_for_log_lock) {
	auto r = runChild([] {
		char dir[] = "/tmp/rbk-shutdown-test.XXXXXX";
		if (!mkdtemp(dir) || chdir(dir) != 0) {
			return fail("temp dir");
		}
		int              argc   = 1;
		char             name[] = "rbk_tests";
		char*            argv[] = {name, nullptr};
		QCoreApplication app(argc, argv);
		if (!Sd::install(1s)) {
			return fail("install");
		}
		qInstallMessageHandler(generalMsgHandler);

		// stdout: a pipe that nobody reads.
		int out[2];
		if (pipe(out) != 0) {
			return fail("pipe");
		}
		fcntl(out[1], F_SETPIPE_SZ, 4096);
		dup2(out[1], STDOUT_FILENO);

		std::thread([] {
			Sd::Loop    guard{"logger"};
			std::string big(1024, 'x');
			for (;;) {
				qInfo("%s", big.c_str()); // blocks in the print, with the log lock held
			}
		}).detach();
		std::this_thread::sleep_for(200ms);

		auto reg = Sd::onStop([] { say("callback ran"); });
		mark("sent");
		kill(getpid(), SIGTERM);
		std::this_thread::sleep_for(1h);
		return 0;
	});
	BOOST_TEST(r.killedBy(SIGTERM), r.err);
	BOOST_TEST(r.err.find("callback ran") != std::string::npos);
	BOOST_TEST(r.err.find("logger") != std::string::npos);
	auto t = r.since("sent");
	BOOST_TEST((t >= 1.0 && t <= 1.5), "t=" << t);
}

// T13: fork() without exec: SIGTERM to the child kills the child only (default action), and
// does not reach the parent's watcher through the shared pipe.
BOOST_AUTO_TEST_CASE(fork_without_exec) {
	auto r = runChild([] {
		if (!Sd::install(5s)) {
			return fail("install");
		}
		pid_t pid = fork();
		if (pid == 0) {
			kill(getpid(), SIGTERM);
			std::this_thread::sleep_for(1s);
			_exit(3); // not killed: wrong
		}
		int status = 0;
		waitpid(pid, &status, 0);
		if (!WIFSIGNALED(status) || WTERMSIG(status) != SIGTERM) {
			return fail("forked child was not killed by SIGTERM");
		}
		std::this_thread::sleep_for(200ms);
		if (!Sd::running()) {
			return fail("the child's SIGTERM stopped the parent");
		}
		return 0;
	});
	BOOST_TEST(r.exited(0), r.err);
}

// T14: finish() flushes stdio and exits with the code, without atexit handlers.
BOOST_AUTO_TEST_CASE(finish_skips_atexit) {
	auto r = runChild([] {
		std::atexit([] { say("atexit ran"); });
		dup2(STDERR_FILENO, STDOUT_FILENO);
		std::printf("buffered tail"); // no newline: only a flush writes it
		return Sd::finish(7);
	});
	BOOST_TEST(r.exited(7), r.err);
	BOOST_TEST(r.err.find("buffered tail") != std::string::npos, r.err);
	BOOST_TEST(r.err.find("atexit ran") == std::string::npos, r.err);
}

BOOST_AUTO_TEST_SUITE_END()
