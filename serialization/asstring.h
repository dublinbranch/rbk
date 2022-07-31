#ifndef ASSTRING_H
#define ASSTRING_H

#include <QDebug>
#include <QList>
#include <QString>

template <class T>
QString asString(QList<T> t) {
	QString cry;
	QDebug(&cry) << t;
	return cry;
}

#endif // ASSTRING_H
