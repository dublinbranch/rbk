#include "extra.h"
#include <QByteArray>
#include <boost/mustache.hpp>

std::string mustache(std::string_view raw, const boost::json::object& json) {
	std::string buffer;
	mustache(raw, buffer, json);
	return buffer;
}

void mustache(std::string_view raw, std::string& buffer, const boost::json::object& json) {
	boost::mustache::render(raw, buffer, json, {});
}

void mustache(const QByteArray& raw, std::string& buffer, const boost::json::object& json) {
	boost::mustache::render(raw.constData(), buffer, json, {});
}
