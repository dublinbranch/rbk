#ifndef THREADSAFEMULTIINDEX_HPP
#define THREADSAFEMULTIINDEX_HPP

#include <boost/multi_index_container.hpp>
#include <mutex>
#include <shared_mutex>

template <typename MultiIndexContainer>
class ThreadSafeMultiIndex {
      public:
	using ContainerType = MultiIndexContainer;
	using ValueType     = typename ContainerType::value_type;

	[[nodiscard("if you discard this the mutex is immediately unlocked, bad!")]]
	std::shared_lock<std::shared_mutex> sharedLock() const {
		return std::shared_lock<std::shared_mutex>(mutex);
	}
	
	[[nodiscard("if you discard this the mutex is immediately unlocked, bad!")]]
	std::unique_lock<std::shared_mutex> uniqueLock() const {
		return std::unique_lock<std::shared_mutex>(mutex);
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
