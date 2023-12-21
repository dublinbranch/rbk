#include "qstringview.h"
#include <QString>

QStringView midView(const QString& string, int pos, int len) {
	return QStringView(string).mid(pos, len);
}
