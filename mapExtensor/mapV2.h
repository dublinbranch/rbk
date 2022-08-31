#pragma once

//we will INTENTIONALLY NOT INCLUDE
//#include "rbk/fmtExtra/includeMe.h"
//as this will include a lot of qt stuff

#include "fmt/core.h"
#include "rbk/QStacker/exceptionv2.h"
#include <map>

template <typename K, typename V, typename _Compare = std::less<K>>
class mapV2 : public std::map<K, V, _Compare> {
      public:
	struct Founded {
		const V* val   = nullptr;
		bool     found = false;
		explicit operator bool() const {
			return found;
		}
	};
	struct Founded2 {
		const V  val   = nullptr;
		bool     found = false;
		explicit operator bool() const {
			return found;
		}
	};

	/*
	 * Use like
	map2<int, string> bla;
	bla.insert({5, "ciao"});

	if (auto f = bla.get(5)) {
	        cout << f.val << "\n";
	} else {
	        cout << "nada\n";
	}
	 */
	[[nodiscard]] auto get(const K& k) const {
		if (auto iter = this->find(k); iter != this->end()) {
			return Founded{&iter->second, true};
		}
		return Founded();
	}

	[[nodiscard]] auto rq(const K& k) const {
		if (auto iter = this->find(k); iter != this->end()) {
			return iter->second;
		}
		throw ExceptionV2(fmt::format("key {} not found in {}", k, __PRETTY_FUNCTION__));
	}

	[[nodiscard]] auto take(const K& k) {
		if (auto iter = this->find(k); iter != this->end()) {
			Founded2 f{iter->second, true};
			this->erase(iter);
			return f;
		}
		return Founded2();
	}

	[[nodiscard]] V getDefault(const K& k) const {
		if (auto v = this->get(k); v) {
			return *(v.val);
		}
		return V();
	}

	[[nodiscard]] V getDefault(const K& k, const V& v) const {
		if (auto found = this->get(k); found) {
			return *(found.val);
		}
		return v;
	}


	bool getOptional(const K& key, V& dest) const {
		if (auto found = get(key); found) {
			dest = *found.val;
			return true;
		}
		return false;
	}

	[[nodiscard]] const auto& operator[](const K& k) const {
		if (auto iter = this->find(k); iter != this->end()) {
			return iter->second;
		} else {
			throw ExceptionV2(fmt::format("key {} not found in {}", k, __PRETTY_FUNCTION__));
		}
	}
	/*
	 *For obscure reason the compiler elect to use the const version, ignoring the base class NON const one
	 * so we redefine ...
	 */
	[[nodiscard]] auto& operator[](const K& k) {
		return std::map<K, V>::operator[](k);
	}

	const V& first() const {
		return this->begin()->second;
	}
};

template <typename K, typename V>
class multiMapV2 : public std::multimap<K, V> {
      public:
	/*
	 * Use like
	map2<int, string> bla;
	bla.insert({5, "ciao"});

	if (auto f = bla.get(5)) {
	        cout << f.val << "\n";
	} else {
	        cout << "nada\n";
	}
	 */
	[[nodiscard]] auto get(const K& k) const {
		struct Founded {
			const V* val   = nullptr;
			bool     found = false;
			         operator bool() const {
				         return found;
			}
		};

		if (auto iter = this->find(k); iter != this->end()) {
			return Founded{&iter->second, true};
		} else {
			return Founded();
		}
	}
};
