#pragma once

//we will INTENTIONALLY NOT INCLUDE
//#include "rbk/fmtExtra/includeMe.h"
//as this will include a lot of qt stuff

#include "NotFoundMixin.h"
#include "rbk/misc/swapType.h"
#include <map>

//void callNotFoundCallback(const QString& key, const std::string location) {
//	(void)key;
//	(void)location;
//}

template <typename K, typename V, typename Compare = std::less<K>>
class mapV2 : public std::map<K, V, Compare>, public NotFoundMixin<K> {
      public:
	using value_type   = std::pair<const K, V>;
	using map_parent   = std::map<K, V, Compare>;
	using mixin_parent = NotFoundMixin<K>;

	mapV2() = default;

	mapV2(std::initializer_list<value_type> map_init)
	    : map_parent(map_init) {
	}

	mapV2(std::initializer_list<value_type> map_init, typename mixin_parent::Funtor f)
	    : map_parent(map_init), mixin_parent(f) {
	}

	struct Founded {
		const V* val   = nullptr;
		bool     found = false;
		explicit operator bool() const {
			return found;
		}
	};
	struct Founded2 {
		const V val   = nullptr;
		bool    found = false;
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

	template <typename T>
	bool get(const K& k, T& t) const {
		if (auto iter = this->find(k); iter != this->end()) {
			swapType(iter->second, t);
			return true;
		}
		return false;
	}

	template <typename T>
	[[nodiscard]] T get(const K& k) const {
		T t;
		if (auto iter = this->find(k); iter != this->end()) {
			swapType(iter->second, t);
		}
		return t;
	}

	[[nodiscard]] V rq(const K& k) const {
		if (auto iter = this->find(k); iter != this->end()) {
			return iter->second;
		}
		this->callNotFoundCallback(k, locationFull());
	}

	template <typename T, std::invocable<K> C>
	[[nodiscard]] T rq(const K& k, C* callback) const {
		if (auto v = get(k); v) {
			return swapType<T>(*(v.val));
		}
		(*callback)(k);
	}

	template <typename T>
	T rq(const K& k) const {
		auto v = rq(k);
		return swapType<T>(v);
	}

	template <class T>
	bool get(const QStringList& keys, T& t) const {
		for (const auto& key : keys) {
			if (get(key, t)) {
				return true;
			}
		}
		return false;
	}

	template <class T>
	void rq(const QStringList& keys, T& t) const {
		for (const auto& key : keys) {
			if (swap(key, t)) {
				return;
			}
		}
		this->callNotFoundCallback(keys.join(" or "), locationFull());
		return;
	}

	[[nodiscard]] V& rqRef(const K& k) {
		if (auto iter = this->find(k); iter != this->end()) {
			return iter->second;
		}
		this->callNotFoundCallback(k, locationFull());
		//just to avoid the warning
		static V v;
		return v;
	}

	template <typename D>
	void rq(const K& key, D& dest) const {
		dest = rq<D>(key);
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

	[[nodiscard]] V getDefault(const K& k, const V& v, bool& isFound) const {
		if (auto found = this->get(k); found) {
			isFound = true;
			return *(found.val);
		}
		isFound = false;
		return v;
	}

	//same interface as normal get!
	bool getOptional(const K& key, V& dest) const {
		if (auto found = get(key); found) {
			dest = *found.val;
			return true;
		}
		return false;
	}

	[[nodiscard]] const auto& operator[](const K& k) const {
		return rqRef(k);
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
class multiMapV2 : public NotFoundMixin<K>, public std::multimap<K, V> {
      public:
	using value_type   = std::pair<const K, V>;
	using map_parent   = std::multimap<K, V>;
	using mixin_parent = NotFoundMixin<K>;

	multiMapV2() = default;

	multiMapV2(std::initializer_list<value_type> map_init)
	    : map_parent(map_init) {
	}

	multiMapV2(std::initializer_list<value_type> map_init, typename mixin_parent::Funtor f)
	    : map_parent(map_init), mixin_parent(f) {
	}

	struct Founded {
		const V* val   = nullptr;
		bool     found = false;

		explicit operator bool() const {
			return found;
		}
	};

	[[nodiscard]] V getDefault(const K& k) const {
		if (auto v = this->get(k); v) {
			return *(v.val);
		}
		return V();
	}

	[[nodiscard]] std::vector<V> rqRange(const K& k) const {
		std::vector<V> res;
		auto           result = this->equal_range(k);
		for (auto it = result.first; it != result.second; it++) {
			res.push_back(it->second);
		}
		if (res.empty()) {
			this->callNotFoundCallback(k, locationFull());
		}
		return res;
	}

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

	template <typename T>
	auto get(const K& k, T& t) const {
		if (auto iter = this->find(k); iter != this->end()) {
			t = swapType<T>(iter->second);
			return true;
		}
		return false;
	}

	[[nodiscard]] auto rq(const K& k) const {
		if (auto iter = this->find(k); iter != this->end()) {
			return iter->second;
		}
		this->callNotFoundCallback(k, locationFull());
		return V();
	}

	template <typename T>
	T rq(const K& k) const {
		auto v = rq(k);
		return swapType<T>(v);
	}

	template <typename T>
	void rq(const K& k, T& t) const {
		auto v = rq(k);
		swapType(v, t);
	}

	template <typename T, typename C>
	[[nodiscard]] T rq(const K& k, C* callback) const {
		if (auto v = get(k); v) {
			return swapType<T>(*(v.val));
		}
		(*callback)(k);

		//this line is normally never execute as the passed lambda supposedly throw a custom exception on miss
		return {};
	}
};
