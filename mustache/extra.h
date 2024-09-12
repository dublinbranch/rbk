#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MUSTACHE_EXTRA_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MUSTACHE_EXTRA_H

#include <boost/json/fwd.hpp>
#include <filesystem>
#include <string>
#include "rbk/filesystem/suffix.h"

class QByteArray;
class Log;

std::string mustache(std::string_view raw, const boost::json::object& json);
std::string mustache(const std::string& raw, const boost::json::object& json);

void        mustache(std::string_view raw, std::string& buffer, const boost::json::object& json);
void        mustache(const QByteArray& raw, std::string& buffer, const boost::json::object& json);
std::string mustache(const QByteArray& raw, const boost::json::object& json);
std::string mustache(const std::filesystem::path& source, const boost::json::object& json);

Log mustacheSudo(const std::filesystem::path& source, const std::filesystem::path& dest, const boost::json::object& json);

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MUSTACHE_EXTRA_H
