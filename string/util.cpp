#include "util.h"
#include <boost/algorithm/string/replace.hpp>

using namespace std;

std::string_view subView(const std::string& string, size_t start, size_t end) {
	std::string_view res(string);
	return res.substr(start, end - start);
}

void replace(const std::string& search, const std::string& replace, std::string& string) {
	boost::algorithm::replace_all(string, search, replace);
}

std::string toStdString(const char *c) {
	return {c};
}

std::string toStdString(const QString& c) {
	return c.toStdString();
}

std::string toStdString(const QByteArray& c) {
	return c.toStdString();
}

string toStdString(const std::string& c) {
	return c;
}

string toStdString(const std::string_view& c) {
	return string(c);
}
