#ifndef RBK_HTTP_LOGINLIMITER_H
#define RBK_HTTP_LOGINLIMITER_H

#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace rbk::Auth {

/**
 * Fail-fast gate in front of a slow password KDF on POST /login.
 *
 * At most two hashes run at once (argon2Live). A second POST for the same IP or
 * the same email is refused even when a global slot is free. Serial guessing is
 * capped by short failure windows.
 */
class LoginLimiter {
      public:
	using Clock = std::function<std::chrono::steady_clock::time_point()>;

	enum class Admit {
		ok,
		inflight, // that IP or email already hashing
		busy,     // argon2Live == kMaxArgon2
		throttled // failure window
	};

	class Guard {
	      public:
		Guard() = default;
		Guard(const Guard&)            = delete;
		Guard& operator=(const Guard&) = delete;
		Guard(Guard&& other) noexcept;
		Guard& operator=(Guard&& other) noexcept;
		~Guard();

		explicit operator bool() const {
			return owner_ != nullptr;
		}

	      private:
		friend class LoginLimiter;
		Guard(LoginLimiter* owner, std::string ip, std::string email);
		void reset();

		LoginLimiter* owner_ = nullptr;
		std::string   ip_;
		std::string   email_;
	};

	struct TryBegin {
		Admit    admit         = Admit::throttled;
		Guard    guard;
		unsigned retryAfterSec = 0;
	};

	static constexpr int      kMaxArgon2             = 2;
	static constexpr int      kIpFailMax             = 10;
	static constexpr int      kEmailFailMax          = 5;
	static constexpr auto     kIpWindow              = std::chrono::minutes(10);
	static constexpr auto     kEmailWindow           = std::chrono::minutes(15);
	static constexpr unsigned kInflightRetryAfterSec = 5;

	explicit LoginLimiter(Clock clock = {});

	static LoginLimiter& instance();

	TryBegin tryBegin(std::string_view ip, std::string_view email);
	void     recordFailure(std::string_view ip, std::string_view email);
	void     recordSuccess(std::string_view email);

	static std::string normalizeEmail(std::string_view email);
	static const char* admitReason(Admit admit);

      private:
	friend class Guard;
	void     release(const std::string& ip, const std::string& email);
	void     pruneLocked(std::vector<std::chrono::steady_clock::time_point>& stamps,
	                     std::chrono::steady_clock::time_point               now,
	                     std::chrono::steady_clock::duration                 window) const;
	unsigned remainingSec(const std::vector<std::chrono::steady_clock::time_point>& stamps,
	                      std::chrono::steady_clock::time_point                     now,
	                      std::chrono::steady_clock::duration                       window) const;
	std::chrono::steady_clock::time_point now() const;

	Clock           clock_;
	std::mutex      mu_;
	std::unordered_set<std::string> inflightIp_;
	std::unordered_set<std::string> inflightEmail_;
	int             argon2Live_ = 0;
	std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> failsByIp_;
	std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> failsByEmail_;
};

} // namespace rbk::Auth

#endif // RBK_HTTP_LOGINLIMITER_H
