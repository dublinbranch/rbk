#include "httpexception.h"

//TODO
// codice duplicato: chiamare costruttore di padre
// vedere se error e data sono usati
HttpException::HttpException(QString _msg) {
	msg = _msg.toUtf8().data();
}

HttpException::HttpException(std::string _msg) {
	msg = QByteArray::fromStdString(_msg).data();
}

HttpException::HttpException(const char* _msg) {
	msg = _msg;
}

const std::string HttpException::getLogFile() const noexcept {
	static const std::string addr = "HttpException.log";
	return addr;
}

void HttpException::HttpParamErrorHandler1(const QString& key) {
	std::string   msg = key.toStdString() + " is not set and is required!";
	HttpException e(msg);
	e.httpErrMsg = msg;
	e.statusCode  = 200;

	throw e;
}

// for testing
void testHttpException() {
	auto e1 = HttpException("e1");

	std::string m3 = "e3";
	auto        e3 = HttpException(m3);

	[[maybe_unused]] int i = 0;
}
