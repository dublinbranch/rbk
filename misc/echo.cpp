#include "echo.h"
#include "fmt/format.h"
#include "rbk/fmtExtra/customformatter.h"
#include <QDebug>
#include <cstdio>
#include <ctime>

void echo(const StringAdt& s) {
	timespec tp{};
	clock_gettime(CLOCK_REALTIME, &tp);
	fmt::print("{}.{:06} {}\n",
	           static_cast<long long>(tp.tv_sec),
	           static_cast<unsigned>(tp.tv_nsec / 1000),
	           s);
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
