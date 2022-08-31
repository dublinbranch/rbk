#include "util.h"
#include "timezone.h"

bool isBefore(const QTime &time) {
	auto now = QDateTime::currentDateTime().time();
	return now < time;
}

ushort getCurHour(const QTimeZone &t) {
	return QDateTime::currentDateTime().toTimeZone(t).time().hour();
}

ushort getCurMinute() {
	return QDateTime::currentDateTime().time().minute();
}

QDateTime getToday() {
	auto      ora = QDate::currentDate();
	QDateTime cry;
	cry.setDate(ora);
	return cry;
}

QDateTime getYesterday() {
	auto      ieri = QDate::currentDate().addDays(-1);
	QDateTime cry;
	cry.setDate(ieri);
	return cry;
}

qint64 getYesterdayTS() {
	return getYesterday().toSecsSinceEpoch();
}

QDateTime getMidnight(QDateTime day) {
	day.setTime({0, 0, 0});
	return day;
}

QDateTime getMidnight(int deltaDays, const QTimeZone* tz) {
	if (!tz) {
		tz = &UTC;
	}
	int       d    = deltaDays * 1;
	auto      ieri = QDate::currentDate().addDays(d);
	QDateTime cry;
	cry.setTimeZone(*tz);
	cry.setDate(ieri);
	return cry;
}

qint64 getMidnightTS(int deltaDays, const QTimeZone* tz) {
	return getMidnight(deltaDays, tz).toSecsSinceEpoch();
}

ushort getCurHour(QTimeZone t) {
	return QDateTime::currentDateTime().toTimeZone(t).time().hour();
}
