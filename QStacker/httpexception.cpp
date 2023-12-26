#include "httpexception.h"
#include "qstacker.h"

//TODO
// codice duplicato: chiamare costruttore di padre
// vedere se error e data sono usati
HttpException::HttpException(QString _msg) {
	msg        = _msg.toUtf8().data();
	httpErrMsg = msg;
}

HttpException::HttpException(const std::string& msg_, const std::string& httpErrMsg_) {
	msg        = msg_;
	httpErrMsg = httpErrMsg_;
}

HttpException::HttpException(const char* _msg) {
	msg        = _msg;
	httpErrMsg = msg;
}

const std::string HttpException::getLogFile() const noexcept {
	static const std::string addr = "HttpException.log";
	return addr;
}

void HttpException::HttpParamErrorHandler1(const QString& key) {
	auto          msg = ">>>" + key + "<<< is not set and is required!";
	HttpException e(msg + "\n" + QStacker16Light());
	e.httpErrMsg = msg.toStdString();
	e.statusCode = 200;

	throw e;
}
