#ifndef HTTPEXCEPTION_H
#define HTTPEXCEPTION_H

#include "exceptionv2.h"
#include <QString>

class HttpException : public ExceptionV2 {
	  public:
	// false -> normal output
	// true -> error message
	bool error = false;
	// normal output
	QByteArray data;

	HttpException(QString _msg, bool _error = true, const QByteArray _data = QByteArray());
	HttpException(std::string _msg, bool _error = true, const QByteArray _data = QByteArray());
	HttpException(const char* _msg, bool _error = true, const QByteArray _data = QByteArray());
};

void testHttpException();

#endif // HTTPEXCEPTION_H
