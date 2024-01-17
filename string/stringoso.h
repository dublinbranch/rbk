#ifndef HOME_ROY_PUBLIC_TESTRTY_STRINGOSO_H
#define HOME_ROY_PUBLIC_TESTRTY_STRINGOSO_H

#include <QByteArray>
#include <QString>
#include <filesystem>
#include <string_view>

class QByteArrayV2 : public QByteArray {
      public:
	QByteArrayV2() = default;
	QByteArrayV2(const QByteArray& input);
	QByteArrayV2(const QString& input);
	QByteArrayV2(const std::string& input);
	QByteArrayV2(const std::string_view& input);
	QByteArrayV2(const char* input);
	QByteArrayV2(const std::filesystem::__cxx11::path& input);
};

//in many places a std::string is requried. so we can not use string_view as the base entity
class StringV2 : public std::string {
      public:
	StringV2(const QByteArray& input);
	StringV2(const QString& input);
	StringV2(const std::string& input);
	StringV2(const std::string_view& input);
	StringV2(const char* input);

      private:
	void setRawData(const char* data, size_t size);
};

class QStringV2 : public QString {
      public:
	QStringV2(const QByteArray& input);
	QStringV2(const QString& input);
	QStringV2(const std::string& input);
	QStringV2(const std::string_view& input);
	QStringV2(const char* input);
	QStringV2(const std::filesystem::__cxx11::path& input);
};
#endif // HOME_ROY_PUBLIC_TESTRTY_STRINGOSO_H
