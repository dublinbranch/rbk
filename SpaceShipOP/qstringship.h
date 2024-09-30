#pragma once

#include <compare>
class QString;
std::strong_ordering operator<=>(const QString& lhs, const QString& rhs);

class QByteArray;
std::strong_ordering operator<=>(const QByteArray& lhs, const QByteArray& rhs);
