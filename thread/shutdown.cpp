#include "shutdown.h"
#include "rbk/misc/QDebugHandler.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>
#endif

namespace rbk::Shutdown {
namespace {

// Everything the watcher touches. Leaked on purpose: the watcher is still alive while static
// destructors run, so none of this may have a static destructor.
struct State {
	std::mutex              m; // guards stopping (write) and every Wakeup::woken
	std::condition_variable cv;
	std::atomic<bool>       stopping{false}; // read anywhere, written only under m
	std::atomic<bool>       installedFlag{false};
	std::atomic<Reason>     reason{Reason::none};
	std::chrono::seconds    deadline{15};
	int                     pipeR = -1;

	std::mutex                                                   regMutex; // callbacks and nextId
	std::vector<std::pair<std::uint64_t, std::function<void()>>> callbacks;
	std::uint64_t                                                nextId = 1;
};

State& st() {
	static State& s = *new State;
	return s;
}

// Names of the live Loop guards. Plain atomics: trivially destructible.
constexpr int                      loopSlotCount = 64;
std::atomic<const char*>           loopSlots[loopSlotCount];
std::atomic<bool>                  loopSlotsFullWarned{false};
constexpr std::chrono::nanoseconds maxWait = std::chrono::hours(24 * 365);

std::chrono::nanoseconds capWait(std::chrono::nanoseconds d) {
	return std::clamp(d, std::chrono::nanoseconds::zero(), maxWait);
}

const char* reasonName(Reason r) {
	switch (r) {
	case Reason::sigterm:
		return "SIGTERM";
	case Reason::sigint:
		return "SIGINT";
	case Reason::request:
		return "request";
	case Reason::none:
		break;
	}
	return "none";
}

#ifndef _WIN32
/* The signal handler only writes one byte to a pipe; the watcher thread reads it. The signal
 * mask is never changed, so child processes (QProcess, system(), popen(), ...) start with the
 * normal SIGINT/SIGTERM behaviour: execve resets caught signals to the default action.
 * Byte 0 is request(); any other byte is the signal number. */
constexpr unsigned char requestByte = 0;
std::atomic<int>        pipeW{-1}; // read by the signal handler: lock-free int
static_assert(std::atomic<int>::is_always_lock_free);

void onSignal(int sig) {
	int  savedErrno = errno;
	auto b          = static_cast<unsigned char>(sig);
	// Non-blocking: if the pipe is full, the watcher already has events to read.
	[[maybe_unused]] auto w = write(pipeW.load(), &b, 1);
	errno                   = savedErrno;
}

// fork() without exec: the child must not share the parent's pipe. It goes back to the
// default signal behaviour and acts as "not installed". Runs in the child after fork(), so
// only async-signal-safe calls. Not called for vfork / posix_spawn (QProcess, system()).
void afterForkInChild() {
	struct sigaction sa{};
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);
	auto& s         = st();
	s.installedFlag = false;
	int w           = pipeW.exchange(-1);
	close(w);
	close(s.pipeR);
	s.pipeR = -1;
}

constexpr int evTimeout     = -1;
constexpr int evInterrupted = -2;

// The next byte from the pipe, evTimeout, or evInterrupted. timeoutMs < 0 waits forever.
int readEvent(int fd, int timeoutMs) {
	unsigned char b = 0;
	if (read(fd, &b, 1) == 1) {
		return b;
	}
	pollfd p{fd, POLLIN, 0};
	int    r = poll(&p, 1, timeoutMs);
	if (r == 0) {
		return evTimeout;
	}
	if (r < 0) {
		return evInterrupted;
	}
	return read(fd, &b, 1) == 1 ? b : evInterrupted;
}

// Other code that sets its own SIGINT/SIGTERM handler after install() (an asio signal_set, an
// MQTT client) silently takes the signal away from the watcher. Warn once per signal.
void checkHandlers() {
	static bool warned[2] = {false, false};
	const int   sigs[2]   = {SIGINT, SIGTERM};
	for (int i = 0; i < 2; ++i) {
		struct sigaction cur{};
		sigaction(sigs[i], nullptr, &cur);
		if (cur.sa_handler != onSignal && !warned[i]) {
			warned[i] = true;
			tryLogLine(true, std::string("shutdown: the ") + (sigs[i] == SIGINT ? "SIGINT" : "SIGTERM") +
			                     " handler was replaced by other code; that signal no longer starts a graceful shutdown");
		}
	}
}

// No qInfo here: the thread that holds the log may be the stuck one.
[[noreturn]] void forcedExit(int sig, std::chrono::steady_clock::time_point started) {
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - started);

	constexpr int cap = 512;
	char          buf[cap];
	int           len   = snprintf(buf, cap, "shutdown: forced after %llds, still running:", static_cast<long long>(elapsed.count()));
	bool          first = true;
	for (auto& slot : loopSlots) {
		const char* name = slot.load();
		if (name && len < cap) {
			len += snprintf(buf + len, cap - len, "%s %s", first ? "" : ",", name);
			first = false;
		}
	}
	if (len < cap - 1) {
		buf[len++] = '\n';
	} else {
		len          = cap;
		buf[len - 1] = '\n';
	}
	[[maybe_unused]] auto w = write(STDERR_FILENO, buf, len);
	tryWriteDiskLog(std::string_view(buf, len));

	// Die by the signal, as if no handler were set.
	signal(sig, SIG_DFL);
	raise(sig);
	// Still alive: for example PID 1 in a container ignores default-action signals it sends
	// itself.
	_exit(128 + sig);
}

void watcherMain() {
	pthread_setname_np(pthread_self(), "shutdown");
	auto& s = st();

	// Wait for the first event. Every 2 s, check that nobody replaced the handlers.
	int ev = evTimeout;
	while (ev < 0) {
		ev = readEvent(s.pipeR, 2000);
		if (ev == evTimeout) {
			checkHandlers();
		}
	}
	auto started = std::chrono::steady_clock::now();

	Reason r       = Reason::sigterm;
	int    exitSig = ev;
	if (ev == requestByte) {
		r       = Reason::request;
		exitSig = SIGTERM;
	} else if (ev == SIGINT) {
		r = Reason::sigint;
	}

	// Before anything that can block.
	alarm(static_cast<unsigned>(s.deadline.count() + 5));

	s.reason = r;
	{
		std::lock_guard lock(s.m);
		s.stopping = true;
	}
	s.cv.notify_all();

	// Wake and interrupt first, log last: logging takes the log lock, and the thread that holds
	// it may be the stuck one.
	std::string errors;
	{
		std::lock_guard lock(s.regMutex);
		for (auto it = s.callbacks.rbegin(); it != s.callbacks.rend(); ++it) {
			try {
				it->second();
			} catch (const std::exception& e) {
				errors += std::string("shutdown: onStop callback threw: ") + e.what() + "\n";
			} catch (...) {
				errors += "shutdown: onStop callback threw\n";
			}
		}
	}

	if (auto* app = QCoreApplication::instance()) {
		// Queued: a direct quit() before exec() is lost, a queued one makes exec() return at once.
		QMetaObject::invokeMethod(app, [] { QCoreApplication::quit(); }, Qt::QueuedConnection);
	}

	// Wait at most 100 ms for the log lock (a normal log call takes microseconds). If it stays
	// busy the line is dropped, so the deadline wait below still runs. A forced exit writes its
	// own line.
	tryLogLine(false, std::string("shutdown: ") + reasonName(r) + " received");
	if (!errors.empty()) {
		errors.pop_back();
		tryLogLine(true, errors);
	}

	// The process normally exits during this wait, and takes the watcher with it.
	// A late request() is ignored; a second signal forces the exit at once.
	auto end = started + s.deadline;
	for (;;) {
		auto remain = std::chrono::ceil<std::chrono::milliseconds>(end - std::chrono::steady_clock::now());
		if (remain <= std::chrono::milliseconds::zero()) {
			break;
		}
		ev = readEvent(s.pipeR, static_cast<int>(remain.count()));
		if (ev < 0 || ev == requestByte) {
			continue;
		}
		break;
	}
	forcedExit(exitSig, started);
}
#endif
} // namespace

bool install(std::chrono::seconds deadline) {
#ifdef _WIN32
	(void)deadline;
	return false;
#else
	auto& s = st();
	if (s.installedFlag) {
		return true;
	}

	int fds[2];
	if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) != 0) {
		qCritical("shutdown: not installed, pipe2 failed: %s", strerror(errno));
		return false;
	}
	s.pipeR    = fds[0];
	pipeW      = fds[1];
	s.deadline = std::max(deadline, std::chrono::seconds(1));

	static bool atforkDone = false;
	if (!atforkDone) {
		atforkDone = true;
		pthread_atfork(nullptr, nullptr, afterForkInChild);
	}

	std::thread(watcherMain).detach();

	// SA_RESTART: most system calls that the signal interrupts on other threads restart. Some
	// (poll, epoll_wait, nanosleep, socket calls with a timeout) still return EINTR once.
	struct sigaction sa{};
	sa.sa_handler = onSignal;
	sa.sa_flags   = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, nullptr);
	sigaction(SIGTERM, &sa, nullptr);

	s.installedFlag = true;
	return true;
#endif
}

bool installed() {
	return st().installedFlag;
}

bool running() {
	return !st().stopping;
}

Reason reason() {
	return st().reason;
}

void request() {
	const auto& s = st();
#ifndef _WIN32
	if (s.installedFlag) {
		if (s.stopping) {
			return;
		}
		unsigned char b = requestByte;
		if (write(pipeW.load(), &b, 1) == 1) {
			return;
		}
		// Pipe full or closed: fall through. The watcher then sees a SIGTERM, so shutdown still
		// starts; only reason() says sigterm instead of request.
	}
	kill(getpid(), SIGTERM);
#else
	(void)s;
	std::raise(SIGTERM);
#endif
}

int finish(int code) {
	fflush(nullptr);
#ifdef _WIN32
	std::_Exit(code);
#else
	_exit(code);
#endif
}

bool sleepFor(std::chrono::nanoseconds d) {
	auto&            s     = st();
	auto             until = std::chrono::steady_clock::now() + capWait(d);
	std::unique_lock lock(s.m);
	s.cv.wait_until(lock, until, [&s] { return s.stopping.load(); });
	return !s.stopping;
}

void Wakeup::notify() {
	auto& s = st();
	{
		std::lock_guard lock(s.m);
		woken = true;
	}
	s.cv.notify_all();
}

Wakeup::Result Wakeup::waitFor(std::chrono::nanoseconds d) {
	auto&            s     = st();
	auto             until = std::chrono::steady_clock::now() + capWait(d);
	std::unique_lock lock(s.m);
	s.cv.wait_until(lock, until, [&] { return s.stopping.load() || woken; });
	if (s.stopping) {
		return Result::stopping;
	}
	if (woken) {
		woken = false;
		return Result::woken;
	}
	return Result::timeout;
}

Registration onStop(std::function<void()> fn) {
	auto&            s = st();
	std::unique_lock lock(s.regMutex);
	// The watcher sets stopping before it takes regMutex, so if stopping is false here, the
	// watcher has not run the callbacks yet and will see this one.
	if (s.stopping) {
		lock.unlock();
		fn();
		return {};
	}
	auto id = s.nextId++;
	s.callbacks.emplace_back(id, std::move(fn));
	return Registration{id};
}

Registration::Registration(std::uint64_t id_)
    : id(id_) {
}

Registration::Registration(Registration&& other) noexcept
    : id(std::exchange(other.id, 0)) {
}

Registration& Registration::operator=(Registration&& other) noexcept {
	if (this != &other) {
		reset();
		id = std::exchange(other.id, 0);
	}
	return *this;
}

Registration::~Registration() {
	reset();
}

void Registration::reset() {
	if (!id) {
		return;
	}
	auto&           s = st();
	std::lock_guard lock(s.regMutex); // waits if the watcher is running the callbacks
	std::erase_if(s.callbacks, [this](const auto& cb) { return cb.first == id; });
	id = 0;
}

Loop::Loop(const char* name) {
#ifndef _WIN32
	char shortName[16] = {};
	snprintf(shortName, sizeof(shortName), "%s", name);
	pthread_setname_np(pthread_self(), shortName);
#endif
	for (int i = 0; i < loopSlotCount; ++i) {
		const char* expected = nullptr;
		if (loopSlots[i].compare_exchange_strong(expected, name)) {
			slot = i;
			return;
		}
	}
	if (!loopSlotsFullWarned.exchange(true)) {
		qWarning("shutdown: more than %d Loop guards, %s is not tracked", loopSlotCount, name);
	}
}

Loop::~Loop() {
	if (slot >= 0) {
		loopSlots[slot] = nullptr;
	}
}

} // namespace rbk::Shutdown
