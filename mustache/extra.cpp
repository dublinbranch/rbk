#include "extra.h"
#include <QByteArray>
#include <boost/mustache.hpp>

using namespace std;

std::string mustache(std::string_view raw, const boost::json::object& json) {
	std::string buffer;
	mustache(raw, buffer, json);
	return buffer;
}

void mustache(std::string_view raw, std::string& buffer, const boost::json::object& json) {
	boost::mustache::render(raw, buffer, json, {});
}

void mustache(const QByteArray& raw, std::string& buffer, const boost::json::object& json) {
    boost::mustache::render(string_view(raw.constData(),(size_t)raw.size()), buffer, json, {});
}

std::string mustache(const QByteArray& raw, const boost::json::object& json) {
	std::string buffer;
	mustache(raw, buffer, json);
	return buffer;
}
