#include "stringoso.h"

QByteAdt::QByteAdt(const QByteArray& input) {
	setRawData(input.data(), input.length());
	detach();
}

QByteAdt::QByteAdt(const QString& input) {
	auto copy = input.toUtf8();
	setRawData(copy.data(), copy.size());
	detach();
}

QByteAdt::QByteAdt(const std::string& input) {
#if QT_VERSION_MAJOR == 5
	setRawData(input.data(), (uint)input.size());
#elif QT_VERSION_MAJOR == 6
	setRawData(input.data(), input.size());
#endif
	detach();
}

QByteAdt::QByteAdt(const std::string_view& input) {
//we intentionally do not call detach because, well is a string view already!
#if QT_VERSION_MAJOR == 5
	setRawData(input.data(), (uint)input.size());
#elif QT_VERSION_MAJOR == 6
	setRawData(input.data(), input.size());
#endif
	detach();
}

QByteAdt::QByteAdt(const char* input) {
	//we intentionally do not call detach because, well is a string view already!
#if QT_VERSION_MAJOR == 5
	setRawData(input, (uint)strlen(input));
#elif QT_VERSION_MAJOR == 6
	setRawData(input, strlen(input));
#endif
	detach();
}

QByteAdt::QByteAdt(const std::filesystem::__cxx11::path& input) {
#if QT_VERSION_MAJOR == 5
	setRawData(input.c_str(), (uint)input.string().size());
#elif QT_VERSION_MAJOR == 6
	setRawData(input.c_str(), input.string().size());
#endif
	detach();
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
	*this = fromUtf8(input);
}

QStringAdt::QStringAdt(const QString& input) {
	setRawData(input.data(), input.size());
	detach();
}

QStringAdt::QStringAdt(const std::string& input) {
	*this = fromStdString(input);
}

QStringAdt::QStringAdt(const std::string_view& input) {
#if QT_VERSION_MAJOR == 5
	append(fromLocal8Bit(input.data(), (uint)input.size()));
#elif QT_VERSION_MAJOR == 6
	append(fromLocal8Bit(input.data(), input.size()));
#endif
}

QStringAdt::QStringAdt(const char* input) {
#if QT_VERSION_MAJOR == 5
	append(fromLocal8Bit(input, (uint)strlen(input)));
#elif QT_VERSION_MAJOR == 6
	append(fromLocal8Bit(input, strlen(input)));
#endif
}

QStringAdt::QStringAdt(const std::filesystem::__cxx11::path& input) {
	*this = fromStdString(input.string());
}
