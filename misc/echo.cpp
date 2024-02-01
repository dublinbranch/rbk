#include "echo.h"
#include "fmt/format.h"
#include "rbk/fmtExtra/customformatter.h"
#include <QDebug>

void echo(const StringViewV2& s) {
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
