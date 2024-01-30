#ifndef HOME_ROY_PUBLIC_TESTRTY_STRINGOSO_H
#define HOME_ROY_PUBLIC_TESTRTY_STRINGOSO_H

#include <QByteArray>
#include <QString>
#include <filesystem>
#include <string_view>

class QByteViewV2 : public QByteArray {
      public:
	QByteViewV2() = default;
	QByteViewV2(const QByteArray& input);
	QByteViewV2(const QString& input);
	QByteViewV2(const std::string& input);
	QByteViewV2(const std::string_view& input);
	QByteViewV2(const char* input);
	QByteViewV2(const std::filesystem::__cxx11::path& input);
};

//in many places a std::string is requried. so we can not use string_view as the base entity
class StringViewV2 : public std::string {
      public:
	StringViewV2(const QByteArray& input);
	StringViewV2(const QString& input);
	StringViewV2(const std::string& input);
	StringViewV2(const std::string_view& input);
	StringViewV2(const char* input);

      private:
	void setRawData(const char* data, size_t size);
};

class QStringViewV2 : public QString {
      public:
	QStringViewV2() = default;
	QStringViewV2(const QByteArray& input);
	QStringViewV2(const QString& input);
	QStringViewV2(const std::string& input);
	QStringViewV2(const std::string_view& input);
	QStringViewV2(const char* input);
	QStringViewV2(const std::filesystem::__cxx11::path& input);
};
#endif // HOME_ROY_PUBLIC_TESTRTY_STRINGOSO_H
