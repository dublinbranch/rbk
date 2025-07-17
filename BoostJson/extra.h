#pragma once

//This file is a disciple of the light shown in https://www.boost.org/doc/libs/1_80_0/libs/json/doc/html/json/dom/conversion.html
#include "rbk/BoostJson/taginvoke.h"
#include "rbk/BoostJson/to_string.h"
#include "rbk/fmtExtra/dynamic.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include "rbk/number/converter.h"
#include "rbk/string/stringoso.h"
#include <boost/json.hpp>
#include <expected>

//there is no arm in doing this
namespace bj = boost::json;

//using namespace std::string_literals; <<-- this one for the suffix s to conver from char[] to std::string

class QString;
class QByteArray;

QString QS(const boost::json::string& cry);
QString QS(const boost::json::string_view& cry);
QString QS(const boost::json::value* value);
QString QS(const boost::json::value& value);
QString QS(const boost::json::value& value, std::string_view key, const QString& def = QString());
/*********/
QByteArray QB(const boost::json::string& cry);
QByteArray QB(const boost::json::string_view& cry);
QByteArray QB(const boost::json::value& value);
QByteArray QB(const boost::json::value& value, std::string_view key, const QByteArray& def = QByteArray());
/********/
std::string_view SW(const boost::json::string_view& cry);

std::string_view                             asString(const boost::json::object& value, const char* key);
std::string_view                             asString(const boost::json::object& value, std::string_view key);
std::expected<std::string_view, std::string> asStringVerbose(const boost::json::object& value, std::string_view key);
std::string_view                             asStringThrow(const boost::json::object& value, std::string_view key);

std::string_view asString(const boost::json::object& value, StringAdt key);
std::string_view asString(const boost::json::object& value, std::string_view key, std::string_view def);

std::string_view asString(const boost::json::value& value, std::string_view key);

std::string_view asString(const boost::json::value& value);

/***********************/
void        pretty_print(std::string& res, boost::json::value const& jv, std::string* indent = nullptr);
std::string pretty_print(boost::json::value const& jv);

QString pretty_printQS(boost::json::value const& jv);

QString serializeQS(boost::json::value const& jv);

//What is the purpose of this stuff ?
template <class T>
boost::json::value J(T&& t) {
	boost::json::value jv;
	boost::json::value_from(std::forward<T>(t), jv);
	return jv;
}

class sqlRow;
boost::json::value asNull(const sqlRow& row, std::string_view key);

bool insertIfNotNull(boost::json::object& target, const sqlRow& row, std::string_view key);

void createOrAppendObj(boost::json::object& json, std::string_view container, std::string_view newElement, const boost::json::value& newValue);

//If needed create the array, else just push into
void pushCreate(boost::json::object& json, std::string_view key, const boost::json::value& newValue);

template <class T>
void pushCreate(boost::json::object& json, std::string_view key, const T& newValue) {
	pushCreate(json, key, boost::json::value_from(newValue));
}

struct JsonRes {
	std::string        raw;
	boost::json::value json;
	//if position is set, it means there was an error and this is the position
	size_t                    position = 0;
	boost::system::error_code ec;
	boost::json::storage_ptr  storage;
	[[nodiscard]] std::string composeErrorMsg() const;
};

JsonRes parseJson(const QByteAdt& json, bool throwOnError = false);
JsonRes parseJson(std::string_view json, bool throwOnError = false);
JsonRes parseJson(const std::string& json, bool throwOnError = false);

std::string escape_json(const std::string& s);
QString     escape_json(const QString& string);

/**
 * @brief getNumber will handle the case when the value is NULL and will use the default instead
 * @param def
 * @return
 */
template <class T>
bool getNumber(const boost::json::value& v, std::string_view key, T& val, const T def = T()) {
	//std::string             block = pretty_print(v);
	//boost::json::error_code e;
	if (auto p = v.as_object().if_contains(key); p) {
		if (p->is_null()) {
			val = def;
			return false;
		}
		if (p->is_string()) {
			//Cry -.-
			//TODO do the actual correct conversion
			val = QS(p).toDouble();
			return true;
		}

		val = p->to_number<T>();
		return true;
	}
	val = def;
	return false;
};

std::string missingKeyError(std::string_view key, bool noThrow);

template <class T>
void rqNumber(const boost::json::value& v, std::string_view key, T& val) {
	if (!getNumber(v, key, val)) {
		missingKeyError(key, false);
	}
}

template <class T>
T rqNumber(const boost::json::value& v, std::string_view key) {
	T val;
	if (!getNumber(v, key, val)) {
		missingKeyError(key, false);
		return 0;
	}
	return val;
}

class DB;
void sqlEscape(boost::json::object& value, DB* db);

std::string join(const boost::json::array& array);

void                     swap(const bj::value& v, std::vector<std::string>& target);
std::vector<std::string> toVecString(const bj::value& v);

template <arithmetic T>
T to_number(const boost::json::value* v) {
	if (v->is_number()) {
		return v->to_number<T>();
	} else if (v->is_string()) {
		auto s = v->as_string();
		return string_to_number<T>(v->as_string());
	} else {
		//auto msg = F("impossible to convert into number {} is a {} : {}", key, item.kind(), pretty_print(item));
		auto msg = F("impossible to convert into number {} is a {} : {}", pretty_print(*v), asSWString(v->kind()));
		throw ExceptionV2(msg);
	}
}

template <arithmetic T>
T to_number(const boost::json::value& v) {
	return to_number<T>(&v);
}

template <arithmetic T>
void to_number(const boost::json::object& v, std::string_view key, T& target) {
	target = to_number<T>(v.at(key));
}

template <arithmetic T>
void to_number(const boost::json::object& v, std::string_view key, std::vector<T>& target) {
	target.clear();
	auto& el = v.at(key);
	if (el.is_array()) {
		auto& arr = el.as_array();
		target.reserve(arr.size());
		for (const auto& item : arr) {
			target.push_back(to_number<T>(item));
		}
	} else {
		target.push_back(to_number<T>(el));
	}
}

template <arithmetic T>
T to_number(const boost::json::object& v, std::string_view key) {
	T t;
	to_number(v, key, t);
	return t;
}

template <typename T>
void rq(const boost::json::value& v, T& target) {
	if constexpr (std::is_same_v<T, bool>) {
		target = v.as_bool();
		return;
	} else if constexpr (std::is_arithmetic_v<T>) {
		if (v.is_string()) {
			target = string_to_number<T>(v.as_string());
		} else {
			target = v.to_number<T>();
		}

		return;
	} else if constexpr (std::is_same_v<T, std::string>) {
		if (v.is_string()) {
			target = v.as_string();
		} else if (v.is_double()) {
			target = F("{}", v.as_double());
		} else if (v.is_int64()) {
			target = F("{}", v.as_int64());
		} else if (v.is_uint64()) {
			target = F("{}", v.as_uint64());
		} else {
			throw ExceptionV2(F("type {} not handled", v.kind()));
		}
	} else {
		// poor man static assert that will also print for which type it failed
		using X = typename T::something_made_up;
		X y;     // To avoid complain that X is defined but not used
		(void)y; // TO avoid complain that y is unused
	}
}

template <typename T>
void rq(const boost::json::object& v, std::string_view key, T& target) {
	rq(v.at(key), target);
}

std::pair<bool, std::string_view> delete_at_pointer(std::string_view sv, boost::json::value* value);

template <typename T>
bool getAtPointer(const bj::value& value, std::string_view ptr, T& t) {
	boost::system::error_code ec;
	if (auto v = value.find_pointer(ptr, ec); v) {
		rq(*v, t);
		return true;
	}
	return false;
}
//bj::value rq(bj::object)

// template <typename T>
// T rq(boost::json::object& obj, std::string_view key) {
// }

// template <typename T>
// T get(boost::json::object& obj, std::string_view key) {
// }

// template <typename T>
// T get(boost::json::object& obj, std::string_view key, const T& def) {
// 	auto* it = obj.if_contains(key);
// 	if (!it) {
// 		return def;
// 	}
// 	switch (it->kind()) {
// 	case bj::kind::object: {
// 		throw ExceptionV2("this is an object!");
// 	}

// 	case bj::kind::array: {
// 		throw ExceptionV2("this is an array!");
// 	}

// 	case bj::kind::string: {
// 		//atm we only support string_view
// 		if constexpr (std::is_same_v<T, std::string_view>) {
// 			return it->get_string();
// 		}
// 		break;
// 	}
// 	case bj::kind::int64:
// 	case bj::kind::double_:
// 	case bj::kind::uint64:
// 		return it->to_number<T>();

// 	case bj::kind::bool_:
// 		return it->get_bool();

// 	case bj::kind::null:
// 		return def;
// 	}
// }
