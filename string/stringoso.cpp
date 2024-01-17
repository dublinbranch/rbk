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
	// Directly accessing the internal data buffer of std::string
	// This is risky and not recommended in standard C++!!!
	// But we like to play with fire no ?
	this->assign(data, size);
}

QStringV2::QStringV2(const QByteArray& input) {
	*this = QString::fromUtf8(input);
}

QStringV2::QStringV2(const QString& input) {
	setRawData(input.data(), input.size());
}

QStringV2::QStringV2(const std::string& input) {
	*this = QString::fromStdString(input);
}

QStringV2::QStringV2(const std::string_view& input) {
	*this = QString::fromLocal8Bit(input.data(), input.size());
}

QStringV2::QStringV2(const char* input) {
	*this = QString::fromLocal8Bit(input, strlen(input));
}

QStringV2::QStringV2(const std::filesystem::__cxx11::path& input) {
	*this = QString::fromStdString(input.string());
}
