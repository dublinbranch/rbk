#include "echo.h"
#include "fmt/format.h"
#include <QDebug>

void echo(const std::string_view s) {
	fmt::print("{}\n", s);
}

void echo(const std::string& s) {
	fmt::print("{}\n", s);
}

void warn(const std::string& msg) {
	QByteArray q;
	q.setRawData(msg.data(), (uint)msg.size());
	qWarning().noquote() << q;
}

void critical(const std::string& msg) {
	QByteArray q;
	q.setRawData(msg.data(), (uint)msg.size());
	qCritical().noquote() << q;
}
