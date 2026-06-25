#include "echo.h"
#include "fmt/format.h"
#include "rbk/fmtExtra/customformatter.h"
#include <QDebug>
#include <cstdio>

void echo(const StringAdt& s) {
	fmt::print("{}\n", s);
	fflush(stdout);
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
