#pragma once

//This file is a disciple of the light shown in https://www.boost.org/doc/libs/1_80_0/libs/json/doc/html/json/dom/conversion.html

#include "rbk/magicEnum/magic_from_string.hpp"
#include <boost/json.hpp>

class QString;
class QByteArray;

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, QString const& t);

void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, QByteArray const& t);

template <isEnum T>
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const T& t) {
	jv = asSWString(t);
}

void        pretty_print(std::string& res, boost::json::value const& jv, std::string* indent = nullptr);
std::string pretty_print(boost::json::value const& jv);

QString QS(const boost::json::string& cry);

QString QS(const boost::json::value* value);
QString QS(const boost::json::value& value);

class sqlRow;
boost::json::value asNull(const sqlRow& row, std::string_view key);

bool insertIfNotNull(boost::json::object &target, const sqlRow& row, std::string_view key);
