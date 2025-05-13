#pragma once
#include <QDateTime>
#include <QTime>
#include <QTimeZone>

//--------------------------------------------------------------------------------------
const QTimeZone UTC        = QTimeZone("UTC");
const QTimeZone LosAngeles = QTimeZone("America/Los_Angeles");

//this should not be here!
const QDateTime unixMidnight = QDateTime::fromSecsSinceEpoch(0, QTimeZone(Qt::UTC));

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

bool isBefore(const QTime& time);

int getCurHour(const QTimeZone& t);

int getCurMinute();

class QDateTime;
QDateTime getToday();
QDateTime getYesterday();
qint64    getYesterdayTS();
/**
 * @brief getMidnightTS
 * @param deltaDays for past day use negative value
 * @param tz
 * @return
 */
qint64 getMidnightTS(int deltaDays, const QTimeZone* tz = nullptr);
/**
 * @brief getMidnight
 * @param deltaDays for past day use negative value
 * @param tz
 * @return
 */
QDateTime getMidnight(int deltaDays, const QTimeZone* tz = nullptr);
QDateTime getMidnight(QDateTime day);
QDateTime getNextMidnight();
qint64    secToNextMidnight();
qint64    getCurrentTS();
qint64    getCurrentMTS();

QString now4file();
