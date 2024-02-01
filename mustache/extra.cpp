#include "extra.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/misc/executor.h"
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
	boost::mustache::render(string_view(raw.constData(), (size_t)raw.size()), buffer, json, {});
}

std::string mustache(const QByteArray& raw, const boost::json::object& json) {
	std::string buffer;
	mustache(raw, buffer, json);
	return buffer;
}

Log mustacheSudo(const std::filesystem::__cxx11::path& source, const std::filesystem::__cxx11::path& dest, const boost::json::object& json) {
	Log l;
	l.section       = __FUNCTION__;
	auto        res = fileGetContents2(source, false);
	std::string buffer;
	mustache(res.content, buffer, json);
	l.push(saveInto(dest, buffer));
	return l;
}
