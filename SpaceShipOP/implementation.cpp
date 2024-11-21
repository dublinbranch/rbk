#include "qdateship.h"
#include "qstringship.h"

#if (QT_VERSION <= QT_VERSION_CHECK(6, 7, 0))

#include "qdateship.h"
#include <QDate>
#include <QString>

std::strong_ordering operator<=>(const QDate& lhs, const QDate& rhs) {
	if (lhs < rhs) {
		return std::strong_ordering::less;
	}
	if (lhs == rhs) {
		return std::strong_ordering::equal;
	}

	return std::strong_ordering::greater;
}

#endif

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))

std::strong_ordering operator<=>(const QString& lhs, const QString& rhs) {
    auto c = lhs.compare(rhs);
    if (c < 0) {
        return std::strong_ordering::less;
    }
    if (c == 0) {
        return std::strong_ordering::equal;
    }
    { //(c > 0)
        return std::strong_ordering::greater;
    }
}

std::strong_ordering operator<=>(const QByteArray& lhs, const QByteArray& rhs) {
    auto c = lhs.compare(rhs);
    if (c < 0) {
        return std::strong_ordering::less;
    }
    if (c == 0) {
        return std::strong_ordering::equal;
    }
    { //(c > 0)
        return std::strong_ordering::greater;
    }
}

#endif
