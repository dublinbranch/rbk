#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

//because GCC do not know clang exists yet
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-var-template"
#endif

template <typename T>
class mi_tls_repository {
      private:
	// Key = memory location of the INSTANCE
	// Value = what you want to store

	using mapT = std::unordered_map<uintptr_t, T>;
	using mapS = std::shared_ptr<mapT>;

	/*
	 * The general per thread map that contain all resources (of this type)
	 * It must be a ptr as it can goes out of scope before the other element, so cleaning will be a disaster
	 */

	static thread_local mapS repository;
	mapS                     local = nullptr;

	void getRepo() {
		return;
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
	mi_tls_repository()
	    : local(repository) {

		// but only the first in this thread will create the map
		getRepo();
	}

	~mi_tls_repository() {
		//		if (repository) {
		//			repository->clear();

		//			delete (repository);

		//			//mark the local instance as clear
		//			repository = nullptr;
		//		}
	}
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

