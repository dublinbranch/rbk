#include "stringoso.h"

QByteAdt::QByteAdt(const QByteArray& input) {
	setRawData(input.data(), input.length());
}

QByteAdt::QByteAdt(const QString& input) {
	auto copy = input.toUtf8();
	setRawData(copy.data(), copy.size());
	detach();
}

QByteAdt::QByteAdt(const std::string& input) {
	setRawData(input.data(), input.size());
}

QByteAdt::QByteAdt(const std::string_view& input) {
	//we intentionally do not call detach because, well is a string view already!
	setRawData(input.data(), input.size());
}

QByteAdt::QByteAdt(const char* input) {
	//we intentionally do not call detach because, well is a string view already!
	setRawData(input, strlen(input));
}

QByteAdt::QByteAdt(const std::filesystem::__cxx11::path& input) {
	setRawData(input.c_str(), input.string().size());
}

StringAdt::StringAdt(const QByteArray& input) {
	setRawData(input.data(), input.size());
}

StringAdt::StringAdt(const QString& input) {
	append(input.toStdString());
}

StringAdt::StringAdt(const std::string& input) {
	setRawData(input.data(), input.size());
}

StringAdt::StringAdt(const std::string_view& input) {
	setRawData(input.data(), input.size());
}

StringAdt::StringAdt(const char* input) {
	setRawData(input, strlen(input));
}

void StringAdt::setRawData(const char* data, size_t size) {
	assign(data, size);
}

QStringAdt::QStringAdt(const QByteArray& input) {
	append(fromUtf8(input));
}

QStringAdt::QStringAdt(const QString& input) {
	setRawData(input.data(), input.size());
}

QStringAdt::QStringAdt(const std::string& input) {
	append(fromStdString(input));
}

QStringAdt::QStringAdt(const std::string_view& input) {
	append(fromLocal8Bit(input.data(), input.size()));
}

QStringAdt::QStringAdt(const char* input) {
	append(fromLocal8Bit(input, strlen(input)));
}

QStringAdt::QStringAdt(const std::filesystem::__cxx11::path& input) {
	append(fromStdString(input.string()));
}
