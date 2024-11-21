#ifndef HOME_ROY_PUBLIC_TESTRTY_STRINGOSO_H
#define HOME_ROY_PUBLIC_TESTRTY_STRINGOSO_H

#include <QByteArray>
#include <QString>
#include <filesystem>
#include <string_view>

class QByteAdt : public QByteArray {
      public:
	QByteAdt() = default;
	QByteAdt(const QByteArray& input);
	QByteAdt(const QString& input);
	QByteAdt(const std::string& input);
	QByteAdt(const std::string_view& input);
	QByteAdt(const char* input);
	QByteAdt(const std::filesystem::__cxx11::path& input);
};

//in many places a std::string is requried. so we can not use string_view as the base entity
class StringAdt : public std::string {
      public:
	StringAdt(const QByteArray& input);
	StringAdt(const QString& input);
	StringAdt(const std::string& input);
	StringAdt(const std::string_view& input);
	StringAdt(const char* input);

      private:
	void setRawData(const char* data, size_t size);
};

class QStringAdt : public QString {
      public:
	QStringAdt() = default;
	QStringAdt(const QByteArray& input);
	QStringAdt(const QString& input);
	QStringAdt(const std::string& input);
	QStringAdt(const std::string_view& input);
	QStringAdt(const char* input);
	QStringAdt(const std::filesystem::__cxx11::path& input);
};
#endif // HOME_ROY_PUBLIC_TESTRTY_STRINGOSO_H
