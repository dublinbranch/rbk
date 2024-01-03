#ifndef HTTPEXCEPTION_H
#define HTTPEXCEPTION_H

#include "exceptionv2.h"
#include <QString>

class HttpException : public ExceptionV2 {
      public:
	unsigned statusCode = 400;
	// if set forced output to be sent as html response
	std::string httpErrMsg;

	HttpException(QString _msg);
	explicit HttpException(const std::string& msg_);
	explicit HttpException(const std::string& msg_, const std::string& httpErrMsg_);
	HttpException(const char* _msg);

	const std::string getLogFile() const noexcept override;

	static void HttpParamErrorHandler1(const QString& key);
};

#endif // HTTPEXCEPTION_H
