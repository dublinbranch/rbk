#include "httpexception.h"

//TODO
// codice duplicato: chiamare costruttore di padre
// vedere se error e data sono usati
HttpException::HttpException(QString _msg, bool _error, const QByteArray _data) {
	msg   = _msg.toUtf8().data();
	error = _error;
	data  = _data;
}

HttpException::HttpException(std::string _msg, bool _error, const QByteArray _data) {
	msg   = QByteArray::fromStdString(_msg).data();
	error = _error;
	data  = _data;
}

HttpException::HttpException(const char* _msg, bool _error, const QByteArray _data) {
	msg   = _msg;
	error = _error;
	data  = _data;
}

// for testing
void testHttpException() {
	auto e1 = HttpException("e1");
	auto e2 = HttpException("e2", false);

	std::string m3 = "e3";
	auto        e3 = HttpException(m3);

	[[maybe_unused]] int i = 0;
}
