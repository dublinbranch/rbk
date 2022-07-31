#pragma once

#include <stdint.h>
#include <unordered_map>

template <typename T>
class mi_tls_repository {
      protected:
	void store(uintptr_t instance, T value) {
		if (!repository) { // check if the application has been already terminated, and we have an out of order destruction
			repository = new std::unordered_map<uintptr_t, T>();
		}
		repository->operator[](instance) = value;
	}

	T& load(uintptr_t instance) const {
		if (!repository) { // check if the application has been already terminated, and we have an out of order destruction
			repository = new std::unordered_map<uintptr_t, T>();
		}
		return repository->operator[](instance);
	}

	void remove(uintptr_t instance) {
		if (!repository) { // check if the application has been already terminated, and we have an out of order destruction
			repository = new std::unordered_map<uintptr_t, T>();
		}
		if (repository->find(instance) != repository->cend()) {
			repository->erase(instance);
		}
	}

	// This will be called for all constructor
	mi_tls_repository() {
		// but only the first in this thread will create the map
		if (!repository) {
			repository = new std::unordered_map<uintptr_t, T>();
		}
	}

	~mi_tls_repository() {
		// Stuff allocated by thread can not be automatically cleaned up as the destruction order can be wrong and so we loose access to the internal stuff
		// Thus is impossible to close the inner content

		// In addition FIRST one in the thread to deallocate trigger this function, thus breaking all the other

		// So STOP wasting time to save a 10Byte memleak, and go figure a better overall logic
	}

      private:
	// Key = memory location of the INSTANCE
	// Value = what you want to store
	// This is manual because thread_local are auto freed, but free order can be wrong (and usually __call_tls_dtors are called before the at_exit handler)! so when the DB closes (and remove the conn) we are in the wrong place
	// Therefore this will leak memory, once you close the program, so is 100% irrelevant

	// We should create another pool managed by us, but is nowhere relavant, as long as you do not create a milion thread...
	inline static thread_local std::unordered_map<uintptr_t, T>* repository = nullptr;
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
