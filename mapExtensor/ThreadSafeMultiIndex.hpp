#ifndef THREADSAFEMULTIINDEX_HPP
#define THREADSAFEMULTIINDEX_HPP

#include <boost/multi_index_container.hpp>
#include <mutex>
#include <shared_mutex>
#include <utility>

namespace rbk {

inline thread_local int threadSafeIndexLockDepth = 0;

[[nodiscard]] inline int mapLockDepth() {
	return threadSafeIndexLockDepth;
}

template <typename Lock>
class DepthTrackedLock {
      public:
	template <typename Mutex>
	explicit DepthTrackedLock(Mutex& mutex)
	    : lock_(mutex)
	    , counted_(lock_.owns_lock()) {
		if (counted_) {
			++threadSafeIndexLockDepth;
		}
	}

	DepthTrackedLock(const DepthTrackedLock&)            = delete;
	DepthTrackedLock& operator=(const DepthTrackedLock&) = delete;

	DepthTrackedLock(DepthTrackedLock&& other) noexcept
	    : lock_(std::move(other.lock_))
	    , counted_(other.counted_) {
		other.counted_ = false;
	}

	~DepthTrackedLock() {
		if (counted_) {
			--threadSafeIndexLockDepth;
		}
	}

	void unlock() {
		lock_.unlock();
		if (counted_) {
			--threadSafeIndexLockDepth;
			counted_ = false;
		}
	}

	[[nodiscard]] bool owns_lock() const {
		return lock_.owns_lock();
	}

      private:
	Lock lock_;
	bool counted_ = false;
};

} // namespace rbk

template <typename MultiIndexContainer>
class ThreadSafeMultiIndex {
      public:
	using ContainerType = MultiIndexContainer;
	using ValueType     = typename ContainerType::value_type;

	[[nodiscard("if you discard this the mutex is immediately unlocked, bad!")]]
	rbk::DepthTrackedLock<std::shared_lock<std::shared_mutex>> sharedLock() const {
		return rbk::DepthTrackedLock<std::shared_lock<std::shared_mutex>>(mutex);
	}

	[[nodiscard("if you discard this the mutex is immediately unlocked, bad!")]]
	rbk::DepthTrackedLock<std::unique_lock<std::shared_mutex>> uniqueLock() const {
		return rbk::DepthTrackedLock<std::unique_lock<std::shared_mutex>>(mutex);
	}

	// Thread-safe size function
	std::size_t size() const {
		std::shared_lock<std::shared_mutex> lock(mutex);
		return container.size();
	}

	mutable std::shared_mutex mutex;
	ContainerType             container;

      private:
};

#endif // THREADSAFEMULTIINDEX_HPP
