#pragma once

//This file is a disciple of the light shown in https://www.boost.org/doc/libs/1_80_0/libs/json/doc/html/json/dom/conversion.html
#include "rbk/BoostJson/taginvoke.h"
#include "rbk/BoostJson/to_string.h"
#include "rbk/magicEnum/magic_from_string.hpp"
#include "rbk/string/stringoso.h"
#include <boost/json.hpp>
#include <optional>

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

std::string_view SW(const boost::json::string_view& cry);

std::string_view asString(const boost::json::object& value, const char* key);
std::string_view asString(const boost::json::object& value, std::string_view key);
std::string_view asString(const boost::json::object& value, StringAdt key);
std::string_view asString(const boost::json::object& value, std::string_view key, std::string_view def);
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
	boost::json::error_code   ec;
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

class DB;
void sqlEscape(boost::json::object& value, DB* db);

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
