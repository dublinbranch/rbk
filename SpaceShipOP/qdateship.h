#pragma once
#include <compare>
class QDate;
std::strong_ordering operator<=>(const QDate& lhs, const QDate& rhs);
