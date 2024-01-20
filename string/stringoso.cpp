#include "stringoso.h"

QByteArrayV2::QByteArrayV2(const QByteArray& input) {
	setRawData(input.data(), input.length());
}

QByteArrayV2::QByteArrayV2(const QString& input) {
	*this = input.toUtf8();
}

QByteArrayV2::QByteArrayV2(const std::string& input) {
	setRawData(input.data(), input.size());
}

QByteArrayV2::QByteArrayV2(const std::string_view& input) {
	//we intentionally do not call detach because, well is a string view already!
	setRawData(input.data(), input.size());
}

QByteArrayV2::QByteArrayV2(const char* input) {
	//we intentionally do not call detach because, well is a string view already!
	setRawData(input, strlen(input));
}

QByteArrayV2::QByteArrayV2(const std::filesystem::__cxx11::path& input) {
	setRawData(input.c_str(), input.string().size());
}

StringV2::StringV2(const QByteArray& input) {
	setRawData(input.data(), input.size());
}

StringV2::StringV2(const QString& input) {
	append(input.toStdString());
}

StringV2::StringV2(const std::string& input) {
	setRawData(input.data(), input.size());
}

StringV2::StringV2(const std::string_view& input) {
	setRawData(input.data(), input.size());
}

StringV2::StringV2(const char* input) {
	setRawData(input, strlen(input));
}

void StringV2::setRawData(const char* data, size_t size) {
	assign(data, size);
}

QStringV2::QStringV2(const QByteArray& input) {
	append(fromUtf8(input));
}

QStringV2::QStringV2(const QString& input) {
	setRawData(input.data(), input.size());
}

QStringV2::QStringV2(const std::string& input) {
	append(fromStdString(input));
}

QStringV2::QStringV2(const std::string_view& input) {
	append(fromLocal8Bit(input.data(), input.size()));
}

QStringV2::QStringV2(const char* input) {
	append(fromLocal8Bit(input, strlen(input)));
}

QStringV2::QStringV2(const std::filesystem::__cxx11::path& input) {
	append(fromStdString(input.string()));
}
