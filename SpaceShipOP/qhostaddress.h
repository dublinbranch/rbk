#pragma once
#include <compare>
class QHostAddress;
std::strong_ordering operator<=>(const QHostAddress& lhs, const QHostAddress& rhs);
