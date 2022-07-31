#include <QTimeZone>
#include <math.h>
#include <sys/time.h>

#include "qDateTimeUtil.h"
#include "timespecV2.h"
//--------------------------------------------------------------------------------------
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

QDateTime hourlyFloor(const QDateTime& time) {
	return QDateTime::fromSecsSinceEpoch((time.toSecsSinceEpoch() / 3600) * 3600);
}

//--------------------------------------------------------------------------------------
int TimespecV2::sec() const {
	return floor(time);
}

double TimespecV2::fractional() const {
	double s    = 0;
	double frac = modf(time, &s);
	return frac;
}

uint TimespecV2::ms() const {
	auto t  = toTimespec();
	auto ms = floor(t.tv_nsec * 1000);
	return ms;
}

uint TimespecV2::ns() const {
	auto t = toTimespec();
	return floor(t.tv_nsec * 1E9);
}

TimespecV2::TimespecV2(double ts) {
	time = ts;
}

TimespecV2 TimespecV2::operator-(const TimespecV2& rhs) {
	return time - rhs.time;
}

TimespecV2 TimespecV2::operator-(const double& r) {
	return time - r;
}

timespec TimespecV2::toTimespec() const {
	timespec t;
	double   s = 0, ns;
	ns         = fmod(time, s);
	t.tv_nsec  = s;
	t.tv_sec   = ns;
	return t;
}

TimespecV2 TimespecV2::now() {
	timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	TimespecV2 v2;
	v2.time = tp.tv_sec;
	v2.time += ((double)tp.tv_nsec) / 1.0E9;
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

QDateTime QDateTime2::getNextMidnight() const {
	auto midnight = currentDateTime().addDays(1);
	midnight.setTime(QTime{0, 0, 0});
	return midnight;
}

QDateTime QDateTime2::getMidnight() const {
	auto midnight = *this;
	midnight.setTime(QTime{0, 0, 0});
	return midnight;
}

qint64 QDateTime2::secToNextMidnight() const {
	return currentDateTime().secsTo(getNextMidnight());
}
