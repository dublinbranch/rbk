#ifndef EXCEPTIONV2_H
#define EXCEPTIONV2_H

#include <QByteArray>
#include <QString>
#include <exception>
#include <source_location>

#define QSL(str) QStringLiteral(str)

class ExceptionV2 : public std::exception {
      public:
	//This is an ugly hack to achieve a weird objective, but is a quite commont techique https://en.wikipedia.org/wiki/Hexspeak
	//We cast the obj and check if start with that to know is ours
	static constexpr ulong uukey     = 0xBADBEEFBADBEEF02;
	const ulong            canaryKey = uukey;
	//This will force the exception to print immediately in case is a "bad error" that we need to be informed about
	bool forcePrint = false;
	//This will SKIP printing when we handle the exception, in case is a minor thing and just save in the log
	bool skipCatchPrint = false;

	ExceptionV2() = default;
	ExceptionV2(const QString& _msg, uint skip = 4);
	ExceptionV2(const char* _msg, uint skip = 4);
	ExceptionV2(const std::string& _msg, uint skip = 4);

	static ExceptionV2 raw(const std::string& _msg);
	static ExceptionV2 location(const std::string& _msg, const std::source_location location =
	                                                         std::source_location::current());

	static ExceptionV2 location(const QString& _msg, const std::source_location location =
	                                                     std::source_location::current());
    
    __attribute__((no_sanitize("address"))) static bool isExceptionV2Derived(void *ptr);
    
	virtual const QString& getLogFile() const noexcept;;
	const char* what() const noexcept override;

	void setMsg(const QByteArray& newMsg);
	void setMsg(const std::string& newMsg);

      protected:
	QByteArray msg;
	QString    logFile = QSL("stdException.log");

      private:
};

const char* currentExceptionTypeName();

/*
To extend do something like

class BadRequestEx : public ExceptionV2 {
      public:
        BadRequestEx(const QString& _msg)
            : ExceptionV2(_msg, 6) {
        }
};

Or for something more creative

class DBException : public ExceptionV2 {
      public:
        enum Error : int {
                NA = 0,
                Connection,
                Warning,
                SchemaError
        } errorType = Error::NA;
        DBException(const QString& _msg, Error error);
};

*/
#endif // EXCEPTIONV2_H
