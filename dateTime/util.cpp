#include "util.h"
#include "rbk/defines/stringDefine.h"
#include "timespecV2.h"
#include <math.h>

bool isBefore(const QTime& time) {
	auto now = QDateTime::currentDateTime().time();
	return now < time;
}

int getCurHour(const QTimeZone& t) {
	return QDateTime::currentDateTime().toTimeZone(t).time().hour();
}

int getCurMinute() {
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
	int       d    = deltaDays;
	auto      ieri = QDate::currentDate().addDays(d);
	QDateTime cry;
	cry.setTimeZone(*tz);
	cry.setDate(ieri);
	return cry;
}

qint64 getMidnightTS(int deltaDays, const QTimeZone* tz) {
	return getMidnight(deltaDays, tz).toSecsSinceEpoch();
}

[[nodiscard]] QDateTime alterTz(const QDateTime& old, const QTimeZone& tz) {
	QDateTime neu;
	neu.setTimeZone(tz);
	neu.setMSecsSinceEpoch(old.toMSecsSinceEpoch());
	return neu;
}

[[nodiscard]] QDateTime setTz(const QDateTime& old, const QTimeZone& tz) {
	QDateTime neu = old;
	neu.setTimeZone(tz);
	return neu;
}

QDateTime hourlyFloor(QDateTime time) {
	auto t = time.time();
	auto h = t.hour();
	t.setHMS(h, 0, 0);
	time.setTime(t);
	return time;
}

//--------------------------------------------------------------------------------------
int TimespecV2::sec() const {
	return static_cast<int>(floor(time));
}

double TimespecV2::fractional() const {
	double s    = 0;
	double frac = modf(time, &s);
	return frac;
}

uint TimespecV2::ms() const {
	auto t  = toTimespec();
	auto ms = floor(t.tv_nsec * 1000);
	return static_cast<uint>(ms);
}

uint TimespecV2::ns() const {
	auto t = toTimespec();
	return static_cast<uint>(floor(static_cast<double>(t.tv_nsec) * 1E9));
}

TimespecV2::TimespecV2(double ts) {
	time = ts;
}

TimespecV2 TimespecV2::operator-(const TimespecV2& rhs) const {
	return time - rhs.time;
}

TimespecV2 TimespecV2::operator-(const double& r) const {
	return time - r;
}

timespec TimespecV2::toTimespec() const {
	timespec t;
	double   s = 0, ns;
	ns         = fmod(time, s);
    t.tv_nsec  = static_cast<long>(s);
    t.tv_sec   = static_cast<time_t>(ns);
	return t;
}

TimespecV2 TimespecV2::now() {
	timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	TimespecV2 v2;
	v2.time = static_cast<double>(tp.tv_sec);
	v2.time += (static_cast<double>(tp.tv_nsec)) / 1.0E9;
	return v2;
}

void TimespecV2::setNow() {
	*this = now();
}

double TimespecV2::toDouble() const {
	return (double)*this;
}

TimespecV2::operator double() const {
	return time;
}

QDateTime getNextMidnight() {
	auto midnight = QDateTime::currentDateTime().addDays(1);
	midnight.setTime(QTime{0, 0, 0});
	return midnight;
}

qint64 secToNextMidnight() {
	return QDateTime::currentDateTime().secsTo(getNextMidnight());
}

QString now4file() {
	return QDateTime::currentDateTime().toString(fileDateTimeFormat);
}
