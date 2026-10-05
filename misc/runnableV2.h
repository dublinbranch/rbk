#pragma once
#include "rbk/number/intTypes.h"
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace rbk {

/** In-memory, in-process version of Runnable (rbk/minMysql/runnable.h): no DB.
 * A restart clears the state. Thread safe. Memory grows by one entry for each distinct key,
 * so use keys from a bounded set (like file:line), not free text.
 *
 * Usable inside the Qt message handler: it does not log.
 * It can throw only std::bad_alloc (copy of a new key, map insert) or std::system_error (mutex lock).
 * In normal conditions this does not happen: the first needs the process to be out of memory, the
 * second needs the OS to refuse a lock on a valid mutex. An exception that leaves the Qt message
 * handler calls std::terminate, but the handler already allocates for each message (fmt, QString),
 * so this adds no new risk. */
class RunnableV2 {
      public:
	struct Result {
		bool run = false;
		// Set only if run: calls blocked since the previous run, and the time of the first one.
		u32 suppressed         = 0;
		i64 firstSuppressedSec = 0;

		explicit operator bool() const {
			return run;
		}
	};

	// for tests: if > 0, used as the current time (seconds since epoch) instead of the system clock
	i64 fakeNowSec = 0;

	/**
	 * @brief runnable checks the last time `key` ran
	 * @param coolDownSec min seconds between two runs, <= 0 = always run
	 * If the clock goes back (now < last run), the call runs: a wrong clock must not block for a long time.
	 */
	[[nodiscard]] Result runnable(std::string_view key, i64 coolDownSec);
	[[nodiscard]] Result operator()(std::string_view key, i64 coolDownSec);

      private:
	struct Entry {
		i64 lastRunSec         = 0;
		u32 suppressed         = 0;
		i64 firstSuppressedSec = 0;
	};
	struct Hash {
		using is_transparent = void;
		size_t operator()(std::string_view s) const noexcept {
			return std::hash<std::string_view>{}(s);
		}
	};

	std::mutex                                                 mu;
	std::unordered_map<std::string, Entry, Hash, std::equal_to<>> entries;
};

} // namespace rbk
