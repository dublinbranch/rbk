#pragma once

//we will INTENTIONALLY NOT INCLUDE
//#include "rbk/fmtExtra/includeMe.h"
//as this will include a lot of qt stuff

#include "NotFoundMixin.h"
#include "rbk/misc/swapType.h"
#include <QDataStream>
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

	friend QDataStream& operator<<(QDataStream& out, const mapV2<K, V, Compare>& map) {
		// Serialize the size of the map first
		out << static_cast<quint32>(map.size());

		// Serialize each key-value pair in the map
		for (const auto& pair : map) {
			out << pair.first << pair.second;
		}

		return out;
	}

	friend QDataStream& operator>>(QDataStream& in, mapV2<K, V, Compare>& map) {
		// Clear the map first to prepare for deserialization
		map.clear();

		// Deserialize the size of the map
		quint32 size;
		in >> size;

		// Deserialize each key-value pair and insert it into the map
		for (quint32 i = 0; i < size; ++i) {
			K key;
			V value;
			in >> key >> value;
			map[key] = value;
		}

		return in;
	}

	struct Founded {
		const V* val   = nullptr;
		bool     found = false;
		explicit operator bool() const {
			return found;
		}
	};

	//Non Constant version
	struct FoundedNC {
		V*       val   = nullptr;
		bool     found = false;
		explicit operator bool() const {
			return found;
		}
	};

	//In V3 replace with std::optional ?
	struct Founded2 {
		const V  val{};
		bool     found = false;
		explicit operator bool() const {
			return found;
		}
	};

	template <typename T>
	struct FoundedV2 {
		const T  val{};
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

	[[nodiscard]] auto getRef(const K& k) {
		if (auto iter = this->find(k); iter != this->end()) {
			return FoundedNC{&iter->second, true};
		}
		return FoundedNC();
	}

	//TODO add the overload for type convertible so we can use the non homogenous map

	//Higher ODR order as the requested type is the same of the one inside the class, so we skip the swap
	bool get(const K& k, V& v) const {
		if (auto iter = this->find(k); iter != this->end()) {
			v = iter->second;
			return true;
		}
		return false;
	}

	template <typename T>
	[[nodiscard]] T get(const K& k, const T&& t) const {
		auto v = t;
		if (auto iter = this->find(k); iter != this->end()) {
			swapType(iter->second, v);
		}
		return v;
	}

	template <typename T>
	bool get(const K& k, T& t) const {
		if (auto iter = this->find(k); iter != this->end()) {
			swapType(iter->second, t);
			return true;
		}
		return false;
	}

	/** TAKE **/
	template <typename T>
	[[nodiscard]] T takeRq(const K& k) {
		auto t = rq<T>(k);
		this->erase(k);
		return t;
	}

	template <typename T>
	[[nodiscard]] T take(const K& k, const T& def) {
		T t = def;
		if (get(k, t)) {
			this->erase(k);
		}
		return t;
	}

	[[nodiscard]] auto takeOpt(const K& k) {
		if (auto iter = this->find(k); iter != this->end()) {
			Founded2 f{iter->second, true};
			this->erase(iter);
			return f;
		}
		return Founded2();
	}
	/** **/

	template <typename T>
	[[nodiscard]] auto get(const K& k) const {
		if (auto iter = this->find(k); iter != this->end()) {
			T t;
			swapType(iter->second, t);
			return FoundedV2<T>{t, true};
		}
		return FoundedV2<T>();
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

	[[nodiscard]] const V& rqRef(const K& k) const {
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
		return this->map_parent::operator[](k);
	}

	const V& first() const {
		return this->begin()->second;
	}

	bool copyIfFound(const K& k, map_parent& source) {
		auto iter = source.find(k);
		auto end  = source.end();

		if (iter != end) {
			this->insert(*iter);
			return true;
		}
		return false;
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

	template <typename T>
	[[nodiscard]] T takeRq(const K& k) {
		auto t = rq<T>(k);
		this->erase(k);
		return t;
	}

	template <typename T>
	[[nodiscard]] T take(const K& k, const T& def) {
		T t = def;
		if (get(k, t)) {
			this->erase(k);
		}
		return t;
	}

	[[nodiscard]] auto takeOpt(const K& k) {
		if (auto iter = this->find(k); iter != this->end()) {
			Founded f{iter->second, true};
			this->erase(iter);
			return f;
		}
		return Founded();
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

	std::vector<K> getAllKeys() const {
		std::vector<K> res;
		for (auto& [k, v] : *this) {
			res.push_back(k);
		}
		return res;
	}
};
