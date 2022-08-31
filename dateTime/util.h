#pragma once
#include <QTime>
#include <QTimeZone>
#include "timezone.h"

bool isBefore(const QTime& time);

ushort getCurHour(const QTimeZone& t);

ushort getCurMinute();

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
ushort    getCurHour(QTimeZone t = UTC);
ushort    getCurMinute();
qint64    getCurrentTS();
qint64    getCurrentMTS();
