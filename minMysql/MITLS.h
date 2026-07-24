#pragma once

#include <cstdint>
#include <unordered_map>

//because GCC do not know clang exists yet
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-var-template"
#endif

template <typename T>
class mi_tls_repository {
      public:
	/**
	 * Per-thread storage for all mi_tls<T> instances on this thread.
	 *
	 * Shutdown ordering: glibc runs __call_tls_dtors() (which destroys repo) at the
	 * top of exit(), BEFORE the atexit handlers that destroy objects with static
	 * storage duration. So a global DB is destroyed after its thread's repo is gone,
	 * and ~mi_tls must not touch the map at that point.
	 *
	 * The "is the repo still alive" flag therefore CANNOT be a member of Repo: a store
	 * to a member from within that object's own destructor is a dead store (nobody may
	 * legally read it afterwards) and the optimizer removes it — which it did, silently
	 * disabling this guard and producing a use-after-free walk of the freed bucket list.
	 * It lives in its own trivially destructible thread_local instead: no destructor is
	 * ever registered for it, so it stays readable for the entire lifetime of the thread.
	 */
	struct Repo {
		std::unordered_map<uintptr_t, T> map;
		~Repo() {
			alive() = false;
		}
	};

      private:
	static thread_local Repo repo;

	// Must stay trivially destructible - see the note on Repo.
	static bool& alive() {
		static thread_local bool flag = true;
		return flag;
	}

      protected:
	void store(uintptr_t instance, T value) {
		if (!alive()) {
			return;
		}
		repo.map[instance] = std::move(value);
	}

	T& load(uintptr_t instance) {
		if (!alive()) {
			thread_local T sink{};
			return sink;
		}
		return repo.map[instance];
	}

	void remove(uintptr_t instance) {
		if (!alive()) {
			return;
		}
		auto it = repo.map.find(instance);
		if (it != repo.map.cend()) {
			repo.map.erase(it);
		}
	}

	mi_tls_repository() = default;

	~mi_tls_repository() = default;
};
#ifdef __clang__
#pragma clang diagnostic pop
#endif

template <typename T>
class mi_tls : protected mi_tls_repository<T> {
      public:
	mi_tls() = default;

	mi_tls(const T& value) {
		this->store(reinterpret_cast<uintptr_t>(this), value);
	}

	mi_tls& operator=(const T& value) {
		this->store(reinterpret_cast<uintptr_t>(this), value);
		return *this;
	}

	T& get() {
		return this->load(reinterpret_cast<uintptr_t>(this));
	}

	//this is pretty cool
	T* operator->() {
		return &this->load(reinterpret_cast<uintptr_t>(this));
	}

	//we want this to NOT be explicit so we can just do something = mi_tls
	//this is a dynamic https://en.cppreference.com/w/cpp/language/cast_operator even cooler
	operator T() {
		return this->load(reinterpret_cast<uintptr_t>(this));
	}

	~mi_tls() {
		this->remove(reinterpret_cast<uintptr_t>(this));
	}
};
