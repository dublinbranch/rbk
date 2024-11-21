#pragma once

#include<QString>

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))

#include <compare>
class QString;
std::strong_ordering operator<=>(const QString& lhs, const QString& rhs);

class QByteArray;
std::strong_ordering operator<=>(const QByteArray& lhs, const QByteArray& rhs);


#endif

