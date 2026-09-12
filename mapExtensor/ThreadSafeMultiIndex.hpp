#ifndef THREADSAFEMULTIINDEX_HPP
#define THREADSAFEMULTIINDEX_HPP

#include <atomic>
#include <boost/multi_index_container.hpp>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <shared_mutex>
#include <string>
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

	explicit DepthTrackedLock(Lock&& lock)
	    : lock_(std::move(lock))
	    , counted_(lock_.owns_lock()) {
		if (counted_) {
			++threadSafeIndexLockDepth;
		}
	}

	template <typename Mutex>
	DepthTrackedLock(Mutex& mutex, std::adopt_lock_t)
	    : lock_(mutex, std::adopt_lock)
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
		if (mutex.try_lock_shared()) {
			sharedAcquires.fetch_add(1, std::memory_order_relaxed);
			return rbk::DepthTrackedLock<std::shared_lock<std::shared_mutex>>(mutex, std::adopt_lock);
		}
		sharedWaits.fetch_add(1, std::memory_order_relaxed);
		rbk::DepthTrackedLock<std::shared_lock<std::shared_mutex>> lock(mutex);
		sharedAcquires.fetch_add(1, std::memory_order_relaxed);
		return lock;
	}

	[[nodiscard("if you discard this the mutex is immediately unlocked, bad!")]]
	rbk::DepthTrackedLock<std::unique_lock<std::shared_mutex>> uniqueLock() const {
		if (mutex.try_lock()) {
			uniqueAcquires.fetch_add(1, std::memory_order_relaxed);
			return rbk::DepthTrackedLock<std::unique_lock<std::shared_mutex>>(mutex, std::adopt_lock);
		}
		uniqueWaits.fetch_add(1, std::memory_order_relaxed);
		rbk::DepthTrackedLock<std::unique_lock<std::shared_mutex>> lock(mutex);
		uniqueAcquires.fetch_add(1, std::memory_order_relaxed);
		return lock;
	}

	// Raw lock on purpose: do not count size() in wait/acquire stats, and do not
	// nest this under sharedLock() — glibc rwlocks are not recursively shareable.
	std::size_t size() const {
		std::shared_lock<std::shared_mutex> lock(mutex);
		return container.size();
	}

	[[nodiscard]] static std::string waitRate(std::uint64_t waits, std::uint64_t acquires) {
		if (!acquires) {
			return "—";
		}
		char buf[32];
		std::snprintf(buf, sizeof(buf), "%.2f%%",
		              100.0 * static_cast<double>(waits) / static_cast<double>(acquires));
		return buf;
	}

	// Atomics only. Safe to call while holding this map's lock.
	[[nodiscard]] std::string lockStatsLine() const {
		const auto   sa = sharedAcquires.load(std::memory_order_relaxed);
		const auto   ua = uniqueAcquires.load(std::memory_order_relaxed);
		const auto   sw = sharedWaits.load(std::memory_order_relaxed);
		const auto   uw = uniqueWaits.load(std::memory_order_relaxed);
		const double sr = sa ? (100.0 * static_cast<double>(sw) / static_cast<double>(sa)) : 0.0;
		const double ur = ua ? (100.0 * static_cast<double>(uw) / static_cast<double>(ua)) : 0.0;
		char         buf[192];
		std::snprintf(buf, sizeof(buf),
		              "shared %llu acq %llu waits (%.2f%%), unique %llu acq %llu waits (%.2f%%)",
		              static_cast<unsigned long long>(sa), static_cast<unsigned long long>(sw), sr,
		              static_cast<unsigned long long>(ua), static_cast<unsigned long long>(uw), ur);
		return buf;
	}

	mutable std::shared_mutex mutex;
	ContainerType             container;

	// After container so these do not share a cache line with mutex.
	mutable std::atomic<std::uint64_t> sharedAcquires{0};
	mutable std::atomic<std::uint64_t> uniqueAcquires{0};
	mutable std::atomic<std::uint64_t> sharedWaits{0};
	mutable std::atomic<std::uint64_t> uniqueWaits{0};
};

#endif // THREADSAFEMULTIINDEX_HPP
