#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MUSTACHE_EXTRA_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MUSTACHE_EXTRA_H

#include <boost/json/fwd.hpp>
#include <string>

class QByteArray;

std::string mustache(std::string_view raw, const boost::json::object& json);

void mustache(std::string_view raw, std::string& buffer, const boost::json::object& json);
void mustache(const QByteArray& raw, std::string& buffer, const boost::json::object& json);

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MUSTACHE_EXTRA_H
