#pragma once

#include <chrono>
#include <cstdint>
#include <functional>

/* Graceful shutdown on SIGTERM / SIGINT. Design: PPPLC todo/graceful-shutdown.md.
 *
 * install() sets a SIGINT/SIGTERM handler that only writes one byte to a pipe, and starts a
 * watcher thread that reads the pipe. The signal mask is not changed, so child processes keep
 * the normal SIGTERM behaviour. On the first signal (or request()) the watcher:
 *   1. arms alarm(deadline + 5) as a backstop,
 *   2. sets running() = false and wakes every sleepFor / Wakeup::waitFor,
 *   3. runs the onStop callbacks, in reverse order of registration,
 *   4. queues QCoreApplication::quit().
 * Loops return, main's exec() returns, tv.wait() joins, main returns.
 * If the process is still alive after `deadline`, or a second signal comes, the watcher logs
 * the names of the Loop guards still alive and kills the process with the first signal.
 *
 * Opt-in: an app that never calls install() behaves as before (Beast catches the signal and
 * re-raises it).
 */
namespace rbk::Shutdown {
enum class Reason { none,
	                sigterm,
	                sigint,
	                request };

// Call right after QCoreApplication is constructed, before Beast::listen() and before other
// code that handles SIGINT/SIGTERM. Sets the handlers and starts the watcher. Returns false
// (and changes nothing) only if the pipe cannot be created; always false on Windows.
// Code that later sets its own SIGINT/SIGTERM handler takes the signal away: the watcher
// logs a warning within 2 s.
bool install(std::chrono::seconds deadline = std::chrono::seconds(15));
bool installed();

bool   running(); // false once shutdown has started
Reason reason();

// Start shutdown from code. Only wakes the watcher; never runs callbacks.
// Ignored once shutdown has started. Not installed: kill(getpid(), SIGTERM).
void request();

// End the process after main's explicit cleanup: flush stdio, then _exit(code). Static
// destructors and atexit handlers do not run (design §4.8 B): they may run while detached
// threads still use the objects they destroy. Use as the last line of main:
// `return rbk::Shutdown::finish(0);`
[[noreturn]] int finish(int code);

// Sleep up to d. Returns false if shutdown has started, at once or during the sleep.
bool sleepFor(std::chrono::nanoseconds d);

// Move-only. Default-constructed = empty. The destructor unregisters, and waits if the
// callback is running.
class Registration {
	  public:
	Registration() = default;
	explicit Registration(std::uint64_t id_);
	Registration(Registration&& other) noexcept;
	Registration& operator=(Registration&& other) noexcept;
	Registration(const Registration&)            = delete;
	Registration& operator=(const Registration&) = delete;
	~Registration();

	  private:
	void          reset();
	std::uint64_t id = 0;
};

// fn runs once when shutdown starts, on the watcher, in reverse order of registration.
// It may only wake or interrupt something (notify, ioc.stop()). It must not block, and must
// not take a lock that the owner may hold while destroying the Registration.
// If shutdown has already started, fn runs at once on the caller's thread.
[[nodiscard]] Registration onStop(std::function<void()> fn);

// For a thread that owns an io_context (io_context::stop() is thread-safe).
template <class Ioc>
[[nodiscard]] Registration stopOnShutdown(Ioc& ioc) {
	return onStop([&ioc] { ioc.stop(); });
}

// A wait that other code can cut short, for loops and queue workers.
// notify() is sticky: a notify with nobody waiting makes the next waitFor return at once.
class Wakeup {
	  public:
	enum class Result { woken,
		                timeout,
		                stopping };
	void   notify();
	Result waitFor(std::chrono::nanoseconds d); // stopping wins over woken

	  private:
	bool woken = false; // guarded by the shared shutdown mutex
};

// First line of every long-lived thread. Sets the thread name (15 chars max) and marks the
// thread alive until the guard is destroyed. A forced exit names the threads still alive.
// name must outlive the guard (use a string literal).
class Loop {
	  public:
	explicit Loop(const char* name);
	~Loop();
	Loop(const Loop&)            = delete;
	Loop& operator=(const Loop&) = delete;

	  private:
	int slot = -1;
};
} // namespace rbk::Shutdown
