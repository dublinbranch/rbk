#pragma once

//This file is a disciple of the light shown in https://www.boost.org/doc/libs/1_80_0/libs/json/doc/html/json/dom/conversion.html

#include "rbk/magicEnum/magic_from_string.hpp"
#include <boost/json.hpp>
#include <optional>

//namespace bj = boost::json;
//using namespace std::string_literals; <<-- this one for the suffix s to conver from char[] to std::string

class QString;
class QByteArray;

QString QS(const boost::json::string& cry);
QString QS(const boost::json::value* value);
QString QS(const boost::json::value& value);

QString QS(const boost::json::value& value, std::string_view key);

std::string asString(const boost::json::value& value);

/** FROM */
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QString& t);
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QByteArray& t);
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QStringList& t);

template <isEnum T>
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const T& t) {
	jv = asSWString(t);
}

/** TO */

QString    tag_invoke(const boost::json::value_to_tag<QString>&, const boost::json::value& jv);
QByteArray tag_invoke(const boost::json::value_to_tag<QByteArray>&, const boost::json::value& jv);

/***********************/
void        pretty_print(std::string& res, boost::json::value const& jv, std::string* indent = nullptr);
std::string pretty_print(boost::json::value const& jv);

QString pretty_printQS(boost::json::value const& jv);

QString serializeQS(boost::json::value const& jv);

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

JsonRes parseJson(const QString& json);
JsonRes parseJson(const QByteArray& json);
JsonRes parseJson(std::string_view json);

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
		val = p->to_number<T>();
		return true;
	}
	val = def;
	return false;
};

class DB;
void sqlEscape(boost::json::object& value, DB* db);
