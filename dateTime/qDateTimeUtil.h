#ifndef QDATETIMEUTIL_H
#define QDATETIMEUTIL_H

#include <QDateTime>
#include <QTimeZone>

//--------------------------------------------------------------------------------------
const QTimeZone UTC        = QTimeZone("UTC");
const QTimeZone LosAngeles = QTimeZone("America/Los_Angeles");

//this should not be here!
const QDateTime unixMidnight = QDateTime::fromSecsSinceEpoch(0, Qt::UTC);

//--------------------------------------------------------------------------------------
//This will keep the timestamp
[[nodiscard]] [[deprecated("use QdateTime.toTimeZone")]] QDateTime alterTz(const QDateTime& old, const QTimeZone& tz);

//This will alter the timestamp!
[[nodiscard]] QDateTime setTz(const QDateTime& old, const QTimeZone& tz);

//Floor to the begin of the hour
[[nodiscard]] QDateTime hourlyFloor(QDateTime time);

//--------------------------------------------------------------------------------------
//small fuction for computin the seconds till the midnight if the current day
qint64 getSecondsUntilMidnight(const QTimeZone& time_zone);

class QDateTime2 : public QDateTime {
      public:
	QDateTime2(const QDateTime& val)
	    : QDateTime(val){};
	QDateTime2() = default;
	QDateTime getNextMidnight() const;
	QDateTime getMidnight() const;
	qint64    secToNextMidnight() const;
};

#endif // QDATETIMEUTIL_H
