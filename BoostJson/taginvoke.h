#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_TAGINVOKE_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_TAGINVOKE_H

#include "rbk/concept/concepts.h"
#include <QtContainerFwd>
#include <boost/json/fwd.hpp>
#include <fmt/printf.h>

class QTime;

//there is no arm in doing this
namespace bj = boost::json;

/** FROM */
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QString& t);
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QByteArray& t);
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QStringList& t);
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const QTime& t);

template <isEnum T>
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const T& t) {
	jv = asSWString(t);
}

/** TO */

QString    tag_invoke(const boost::json::value_to_tag<QString>&, const boost::json::value& jv);
QByteArray tag_invoke(const boost::json::value_to_tag<QByteArray>&, const boost::json::value& jv);
QTime      tag_invoke(bj::value_to_tag<QTime>, bj::value const& v);






#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_TAGINVOKE_H
