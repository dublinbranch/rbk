#include "qstring.h"

QString Q16(std::string s) {
	return QString::fromStdString(s);
}

QByteArray Q8(std::string s) {
	return QByteArray::fromStdString(s);
}
