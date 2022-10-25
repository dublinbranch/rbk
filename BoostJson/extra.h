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
	jv = {asSWString(t)};
}
