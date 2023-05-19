#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

template <typename T>
class mi_tls_repository {
      private:
	using mapT = std::unordered_map<uintptr_t, T>;

	inline static thread_local mapT* repository = nullptr;
	std::vector<mapT*>               repoList;
	inline static std::mutex         m;

	void getRepo() {
		if (!repository) {
			repository = new mapT();
			std::lock_guard<std::mutex> l(m);
			repoList.push_back(repository);
		}
	}

      protected:
	void store(uintptr_t instance, T value) {
		getRepo();
		repository->operator[](instance) = value;
	}

	T& load(uintptr_t instance) {
		getRepo();
		return repository->operator[](instance);
	}

	void remove(uintptr_t instance) {
		getRepo();
		if (repository->find(instance) != repository->cend()) {
			repository->erase(instance);
		}
	}

	// This will be called for all constructor
	mi_tls_repository() {
		// but only the first in this thread will create the map
		getRepo();
	}

	~mi_tls_repository() {
		//only one destructor per thread has to run
		std::lock_guard l(m);
		if (repoList.empty()) {
			return;
		}

		for (auto& row : repoList) {
			row->clear();
			delete (row);
		}
		repoList.clear();
	}

      private:
	// Key = memory location of the INSTANCE
	// Value = what you want to store
	// This is manual because thread_local are auto freed, but free order can be wrong (and usually __call_tls_dtors are called before the at_exit handler)! so when the DB closes (and remove the conn) we are in the wrong place
	// Therefore this will leak memory, once you close the program, so is 100% irrelevant

	// We should create another pool managed by us, but is nowhere relavant, as long as you do not create a milion thread...
};

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

	operator T() {
		return this->load(reinterpret_cast<uintptr_t>(this));
	}

	~mi_tls() {
		this->remove(reinterpret_cast<uintptr_t>(this));
	}
};
