#pragma once

#include "fmt/core.h"
#include "rbk/QStacker/exceptionv2.h"
#include <unordered_map>

template <typename K, typename V, typename _Hash = std::hash<K>>
class hmap : public std::unordered_map<K, V, _Hash> {
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

	bool get(const K& k, V& v) const {
		if (auto iter = this->find(k); iter != this->end()) {
			v = iter->second;
			return true;
		} else {
			return false;
		}
	}

	[[nodiscard]] auto getNC(const K& k) {
		struct Founded {
			V*   val   = nullptr;
			bool found = false;

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
		return std::unordered_map<K, V>::operator[](k);
	}

	const V& first() const {
		return this->begin()->second;
	}
};
