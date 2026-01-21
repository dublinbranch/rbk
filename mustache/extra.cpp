#include "extra.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/filesystem/filefunction.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/misc/executor.h"
#include <QByteArray>
#include <boost/mustache.hpp>
#include <qdebug.h>

using namespace std;

std::string mustache(std::string_view raw, const boost::json::object& json) {
	std::string buffer;
	mustache(raw, buffer, json);
	return buffer;
}

string mustache(const std::string& raw, const boost::json::object& json) {
	return mustache(string_view(raw), json);
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

#ifdef WITH_REPROC
Log mustacheSudo(const std::filesystem::__cxx11::path& source, const std::filesystem::__cxx11::path& dest, const boost::json::object& json) {
	Log l;
	l.section  = __PRETTY_FUNCTION__;
	l.category = Log::Exception;

	auto        res = fileGetContents2(source, false);
	std::string buffer;
	mustache(res.content, buffer, json);
	l.push(saveInto(dest, buffer));

	l.category = Log::Info;
	return l;
}
#endif

string mustache(const std::filesystem::__cxx11::path& source, const boost::json::object& json) {
	auto res = fileGetContents2(source, true);
	if (!res) {
		auto msg = F16("impossible to find {} in {}", source.string(), QStacker16Light());
		qCritical().noquote() << msg;
		return msg.toStdString();
	}
	return mustache(res.content, json);
}

string mustache(std::string_view raw, const boost::json::value& json) {
	string buffer;
	boost::mustache::render(raw, buffer, json, {});
	return buffer;
}
