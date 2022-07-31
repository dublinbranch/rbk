#pragma once
#include <compare>
class QString;
std::strong_ordering operator<=>(const QString& lhs, const QString& rhs);
