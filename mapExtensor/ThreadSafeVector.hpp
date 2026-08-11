#ifndef THREADSAFEVECTOR_HPP
#define THREADSAFEVECTOR_HPP

#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

/**
 * @brief Mutex-backed vector with an RAII locked view for compound ops.
 *
 * Prefer locked() for multi-step work (find + push + trim, snapshot then use
 * outside the lock). One-shot helpers (push_back, size, ...) lock for the whole call.
 * Do not call one-shot helpers while already holding Locked — mutex is not recursive.
 *
 * Optional maxSize: when non-zero, push_back drops from the front (oldest) so size
 * never exceeds the cap (same idea as RRList).
 */
template <typename T>
class ThreadSafeVector {
      public:
	using value_type = T;
	using Container  = std::vector<T>;

	explicit ThreadSafeVector(std::size_t maxSize = 0)
	    : maxSize_(maxSize) {
	}

	ThreadSafeVector(const ThreadSafeVector&)            = delete;
	ThreadSafeVector& operator=(const ThreadSafeVector&) = delete;
	ThreadSafeVector(ThreadSafeVector&&)                 = delete;
	ThreadSafeVector& operator=(ThreadSafeVector&&)      = delete;

	class Locked {
	      public:
		Locked(const Locked&)            = delete;
		Locked& operator=(const Locked&) = delete;
		Locked(Locked&&)                 = default;
		Locked& operator=(Locked&&)      = default;

		Container&       get() { return owner_.data_; }
		const Container& get() const { return owner_.data_; }

		Container*       operator->() { return &owner_.data_; }
		const Container* operator->() const { return &owner_.data_; }
		Container&       operator*() { return owner_.data_; }
		const Container& operator*() const { return owner_.data_; }

		void push_back(const T& value) {
			owner_.data_.push_back(value);
			owner_.trimToMaxSize();
		}
		void push_back(T&& value) {
			owner_.data_.push_back(std::move(value));
			owner_.trimToMaxSize();
		}

		template <typename... Args>
		T& emplace_back(Args&&... args) {
			T& ref = owner_.data_.emplace_back(std::forward<Args>(args)...);
			owner_.trimToMaxSize();
			return owner_.data_.back();
		}

		[[nodiscard]] std::size_t size() const { return owner_.data_.size(); }
		[[nodiscard]] bool        empty() const { return owner_.data_.empty(); }
		void                      clear() { owner_.data_.clear(); }

	      private:
		friend class ThreadSafeVector;
		explicit Locked(ThreadSafeVector& owner)
		    : owner_(owner)
		    , lock_(owner.mutex_) {
		}

		ThreadSafeVector&            owner_;
		std::unique_lock<std::mutex> lock_;
	};

	class ConstLocked {
	      public:
		ConstLocked(const ConstLocked&)            = delete;
		ConstLocked& operator=(const ConstLocked&) = delete;
		ConstLocked(ConstLocked&&)                 = default;
		ConstLocked& operator=(ConstLocked&&)      = default;

		const Container& get() const { return owner_.data_; }
		const Container* operator->() const { return &owner_.data_; }
		const Container& operator*() const { return owner_.data_; }

		[[nodiscard]] std::size_t size() const { return owner_.data_.size(); }
		[[nodiscard]] bool        empty() const { return owner_.data_.empty(); }

	      private:
		friend class ThreadSafeVector;
		explicit ConstLocked(const ThreadSafeVector& owner)
		    : owner_(owner)
		    , lock_(owner.mutex_) {
		}

		const ThreadSafeVector&      owner_;
		std::unique_lock<std::mutex> lock_;
	};

	[[nodiscard("if you discard this the mutex is immediately unlocked, bad!")]]
	Locked locked() {
		return Locked(*this);
	}

	[[nodiscard("if you discard this the mutex is immediately unlocked, bad!")]]
	ConstLocked locked() const {
		return ConstLocked(*this);
	}

	void push_back(const T& value) {
		locked().push_back(value);
	}
	void push_back(T&& value) {
		locked().push_back(std::move(value));
	}

	template <typename... Args>
	void emplace_back(Args&&... args) {
		locked().emplace_back(std::forward<Args>(args)...);
	}

	[[nodiscard]] std::size_t size() const {
		return locked().size();
	}
	[[nodiscard]] bool empty() const {
		return locked().empty();
	}
	void clear() {
		locked().clear();
	}

	/** Copy under lock; use for snapshot-then-process-outside-lock. */
	[[nodiscard]] Container copy() const {
		auto g = locked();
		return g.get();
	}

	[[nodiscard]] std::size_t maxSize() const {
		return maxSize_;
	}

      private:
	void trimToMaxSize() {
		if (!maxSize_) {
			return;
		}
		while (data_.size() > maxSize_) {
			data_.erase(data_.begin());
		}
	}

	mutable std::mutex mutex_;
	Container          data_;
	std::size_t        maxSize_ = 0;
};

#endif // THREADSAFEVECTOR_HPP
