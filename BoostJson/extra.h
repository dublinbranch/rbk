#pragma once

//This file is a disciple of the light shown in https://www.boost.org/doc/libs/1_80_0/libs/json/doc/html/json/dom/conversion.html

#include "rbk/magicEnum/magic_from_string.hpp"
#include <boost/json.hpp>

//namespace bj = boost::json;
//using namespace std::string_literals;

class QString;
class QByteArray;

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, QString const& t);
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, QByteArray const& t);
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, QStringList const& t);

template <isEnum T>
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const T& t) {
	jv = asSWString(t);
}

void        pretty_print(std::string& res, boost::json::value const& jv, std::string* indent = nullptr);
std::string pretty_print(boost::json::value const& jv);

QString pretty_printQS(boost::json::value const& jv);

QString serializeQS(boost::json::value const& jv);

QString QS(const boost::json::string& cry);
QString QS(const boost::json::value* value);
QString QS(const boost::json::value& value);

class sqlRow;
boost::json::value asNull(const sqlRow& row, std::string_view key);

bool insertIfNotNull(boost::json::object& target, const sqlRow& row, std::string_view key);

//If needed create the array, else just push into
void pushCreate(boost::json::object& value, std::string_view key, const boost::json::value& newValue);

template <class T>
void pushCreate(boost::json::object& value, std::string_view key, const T& newValue) {
	pushCreate(value, key, boost::json::value_from(newValue));
}

struct JsonRes {
	std::string        raw;
	boost::json::value json;
	//if position is set, it means there was an error and this is the position
	uint                    position = 0;
	boost::json::error_code ec;
	std::string             composeErrorMsg() const;
};

JsonRes parseJson(const QByteArray& json);
JsonRes parseJson(std::string_view json);

class DB;
void sqlEscape(boost::json::object& value, DB* db);
