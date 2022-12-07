#include "echo.h"
#include <QDebug>

#include "fmt/format.h"
void echo(const std::string_view s) {
	fmt::print("{}\n", s);
}

void echo(const std::string& s) {
	fmt::print("{}\n", s);
}

void warn(std::string& msg) {
	QByteArray q;
	q.setRawData(msg.data(), msg.size());
	qDebug().noquote() << q;
}

void critical(std::string& msg) {
	QByteArray q;
	q.setRawData(msg.data(), msg.size());
	qDebug().noquote() << q;
}
