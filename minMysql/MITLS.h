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
	 * alive is cleared at the start of ~Repo(); map is destroyed afterward, so
	 * load/store/remove can treat !alive as "shutting down" and avoid touching map.
	 * (If repo is fully destroyed before ~mi_tls, accessing repo is still UB in
	 * theory; ~DB no longer calls closeConn() to avoid that shutdown path.)
	 */
	struct Repo {
		std::unordered_map<uintptr_t, T> map;
		bool                             alive = true;
		~Repo() {
			alive = false;
		}
	};

      private:
	static thread_local Repo repo;

      protected:
	void store(uintptr_t instance, T value) {
		if (!repo.alive) {
			return;
		}
		repo.map[instance] = std::move(value);
	}

	T& load(uintptr_t instance) {
		if (!repo.alive) {
			thread_local T sink{};
			return sink;
		}
		return repo.map[instance];
	}

	void remove(uintptr_t instance) {
		if (!repo.alive) {
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
