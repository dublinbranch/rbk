#ifndef ANKERV2_H
#define ANKERV2_H

#include "NotFoundMixin.h"
#include "missingkeyex.h"
#include "rbk/mapExtensor/ankerl_unordered_dense.h"
#include "rbk/misc/swapType.h"

template <class Key, class Value, class Hash, class Equal>
class AnkerV2 : public ankerl::unordered_dense::map<Key, Value, Hash, Equal>, public NotFoundMixin<Key> {
      public:
	AnkerV2()  = default;
	using Base = ankerl::unordered_dense::map<Key, Value, Hash, Equal>;

	// Friend declaration for serialization
	friend QDataStream& operator<<(QDataStream& out, const AnkerV2& map) {
		// Serialize the size of the map first
		out << static_cast<quint32>(map.size());

		// Serialize each key-value pair
		for (const auto& pair : map) {
			out << pair.first << pair.second; // Assuming Key and Value are serializable with QDataStream
		}

		return out;
	}

	// Friend declaration for deserialization
	friend QDataStream& operator>>(QDataStream& in, AnkerV2& map) {
		// Clear the map first to prepare for deserialization
		map.clear();

		// Read the size of the map
		quint32 size;
		in >> size;

		// Deserialize each key-value pair and insert it into the map
		for (quint32 i = 0; i < size; ++i) {
			Key   key;
			Value value;

			in >> key >> value; // Assuming Key and Value are deserializable with QDataStream
			map[key] = value;
		}

		return in;
	}

	struct Founded {
		const Value* val   = nullptr;
		bool         found = false;

		explicit operator bool() const {
			return found;
		}
	};

	/*
	if(auto v = map.get("x"); v){
	        val = *v.value;
	}
	*/
	template <class KT>
	[[nodiscard]] auto get(const KT& k) const {
		if (auto iter = this->find(k); iter != this->end()) {
			return Founded{&iter->second, true};
		}
		return Founded();
	}

	/*
	Type value;
	bool found = map.get<type>("key",value);
	*/
	template <class KT>
	bool get(const KT& k, Value& v) const {
		if (auto iter = this->find(k); iter != this->end()) {
			v = iter->second;
			return true;
		}
		return false;
	}

	/*
	Value value;
	const Value default;
	map.get<type>("key",value,default);
	*/
	void get(const Key& k, Value& v, const Value& def) const {
		if (auto iter = this->find(k); iter != this->end()) {
			v = iter->second;
		}
		return def;
	}
	/*
	Value default;
	Value value = map.getDef<type>("key",default);
	*/
	Value getDef(const Key& k, const Value& def) const {
		if (auto iter = this->find(k); iter != this->end()) {
			return iter->second;
		}
		return def;
	}

	/*
	const Type default;
	Type value = map.get<type>("key",default);
	*/
	template <class KT, typename T>
	[[nodiscard]] T get(const KT& k, const T&& t) const {
		if (auto iter = this->find(k); iter != this->end()) {
			auto v = t;
			swapType(iter->second, v);
			return v;
		}
		return t;
	}

	/*
	Type value;
	bool found = map.rq<type>("key",value);
	*/
	template <class KT, typename T>
	bool get(const KT& k, T& t) const {
		if (auto iter = this->find(k); iter != this->end()) {
			swapType(iter->second, t);
			return true;
		}
		return false;
	}

	/*
	Value value = map.rq("key");
	*/
	template <class KT>
	[[nodiscard]] Value rq(const KT& k) const {
		if (auto iter = this->find(k); iter != this->end()) {
			return iter->second;
		}
		if constexpr (std::is_same_v<KT, std::string_view>) {
			this->callNotFoundCallback(std::string(k), locationFull());
		} else {
			this->callNotFoundCallback(k, locationFull());
		}
	}

	/*
	Type value = map.rq<Type>("key",callbackOnFail);
	*/
	template <typename KT, typename T, std::invocable<Key> C>
	[[nodiscard]] T rq(const KT& k, C* callback) const {
		if (auto iter = this->find(k); iter != this->end()) {
			return swapType<T>(iter->second);
		}
		(*callback)(k);
	}

	/*
	Type value = map.rq<Type>("key");
	*/
	template <typename T, typename KT>
	T rq(const KT& k) const {
		auto v = rq(k);
		return swapType<T>(v);
	}

	/*
	Type value;
	map.rq("key",valeu);
	*/
	template <class KT, typename T>
	void rq(const KT& key, T& dest) const {
		dest = rq<T>(key);
	}

	/*
	auto value = map.first();
	*/
	const Value& first() const {
		return this->begin()->second;
	}
};

#endif // ANKERV2_H
