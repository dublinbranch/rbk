#include "stringoso.h"

QByteViewV2::QByteViewV2(const QByteArray& input) {
	setRawData(input.data(), input.length());
}

QByteViewV2::QByteViewV2(const QString& input) {
	auto copy = input.toUtf8();
	setRawData(copy.data(), copy.size());
	detach();
}

QByteViewV2::QByteViewV2(const std::string& input) {
	setRawData(input.data(), input.size());
}

QByteViewV2::QByteViewV2(const std::string_view& input) {
	//we intentionally do not call detach because, well is a string view already!
	setRawData(input.data(), input.size());
}

QByteViewV2::QByteViewV2(const char* input) {
	//we intentionally do not call detach because, well is a string view already!
	setRawData(input, strlen(input));
}

QByteViewV2::QByteViewV2(const std::filesystem::__cxx11::path& input) {
	setRawData(input.c_str(), input.string().size());
}

StringViewV2::StringViewV2(const QByteArray& input) {
	setRawData(input.data(), input.size());
}

StringViewV2::StringViewV2(const QString& input) {
	append(input.toStdString());
}

StringViewV2::StringViewV2(const std::string& input) {
	setRawData(input.data(), input.size());
}

StringViewV2::StringViewV2(const std::string_view& input) {
	setRawData(input.data(), input.size());
}

StringViewV2::StringViewV2(const char* input) {
	setRawData(input, strlen(input));
}

void StringViewV2::setRawData(const char* data, size_t size) {
	assign(data, size);
}

QStringViewV2::QStringViewV2(const QByteArray& input) {
	append(fromUtf8(input));
}

QStringViewV2::QStringViewV2(const QString& input) {
	setRawData(input.data(), input.size());
}

QStringViewV2::QStringViewV2(const std::string& input) {
	append(fromStdString(input));
}

QStringViewV2::QStringViewV2(const std::string_view& input) {
	append(fromLocal8Bit(input.data(), input.size()));
}

QStringViewV2::QStringViewV2(const char* input) {
	append(fromLocal8Bit(input, strlen(input)));
}

QStringViewV2::QStringViewV2(const std::filesystem::__cxx11::path& input) {
	append(fromStdString(input.string()));
}
