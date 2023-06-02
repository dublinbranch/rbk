#ifndef DEPLETER_H
#define DEPLETER_H

#include "boost/json.hpp"
#include "rbk/BoostJson/extra.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/fmtExtra/dynamic.h"
#include "rbk/mapExtensor/NotFoundMixin.h"
#include "rbk/mapExtensor/isIterable.h"
#include <concepts>
#include <functional>
#include <string_view>
#include <sys/types.h>
#include <type_traits>

class Depleter {
      public:
	NotFoundMixin<std::string_view> notFound;
	//non owning ptr
	explicit Depleter(boost::json::value* json_);

	Depleter operator[](std::string_view key);
	Depleter operator[](uint pos);

	template <typename V>
	bool get(std::string_view key, V& dest, const V& def) const {
		auto& obj  = json->as_object();
		auto  iter = obj.find(key);
		if (iter == obj.end()) {
			dest = def;
			return false;
		}
		swap(key, iter->value(), dest);
		obj.erase(iter);
		return true;
	}

	template <typename V>
	//this is used mostly to provide a default
	V get(std::string_view key, const V& def) const {
		V dest;
		get(key, dest, def);
		return dest;
	}

	template <typename V>
	V get(std::string_view key) const {
		V dest;
		get(key, dest);
		return dest;
	}

	template <typename V>
	bool get(std::string_view key, V& dest) const {
		return get(key, dest, dest);
	}

	template <typename V>
	void rq(std::string_view key, V& dest) const {
		if (!get(key, dest)) {
			notFound.callNotFoundCallback(key, stacker());
		}
	}

	template <typename V>
	V rq(std::string_view key) const {
		V dest;
		rq(key, dest);
		return dest;
	}

	template <typename V>
	void unroll(std::string_view key, const boost::json::array& array, V& vector) const {
		for (const auto& row : array) {
			typename V::value_type temp;
			swap(key, row, temp);
			vector.push_back(temp);
		}
	}

	  private:
	boost::json::value* json;

	/** I do not think is possible to have a templated customizable function ...
	 * @brief swap
	 * @param key
	 * @param value
	 * @param dest
	 */
	template <typename V>
	void swap(std::string_view key, const boost::json::value& value, V dest) const {
		std::string pp = pretty_print(value);
		//This fail as is resolved to the wrong type (an array ?)
		if constexpr (std::is_same_v<V, QByteArray>) {
			//This fail as is resolved to the wrong type (an array ?)
			//dest = boost::json::value_to<QByteArray>(value);
			auto temp = boost::json::value_to<std::string_view>(value);
			dest.setRawData(temp.data(), temp.size());
			dest.detach();
		} else {
			auto res = boost::json::try_value_to<V>(value);
			if (res.has_error()) {
				auto&& e = res.error();
				//auto   v   = e.value();
				//auto   l   = e.location();
				auto        w   = e.what();
				std::string msg = fmt::format("Impossible to convert {}, reason {} ", key, w);
				throw ExceptionV2(msg);
			}
		}
	}
};

#endif // DEPLETER_H
