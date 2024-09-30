#pragma once

#include<QDate>

#if (QT_VERSION <= QT_VERSION_CHECK(6, 7, 0))

#include <compare>
class QDate;
std::strong_ordering operator<=>(const QDate& lhs, const QDate& rhs);

#endif
