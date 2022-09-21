#include "salt.h"
#include "rbk/misc/b64.h"
#include <QDateTime>

std::string salt(size_t lenght, std::string) {
	return saltQS(lenght).toStdString();
}

QString salt(QString, size_t lenght) {
	return saltQS(lenght);
}

QString saltQS(size_t lenght) {
	return toBase64(QString::number(QDateTime::currentMSecsSinceEpoch())).left(lenght);
}
