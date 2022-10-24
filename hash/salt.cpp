#include "salt.h"
#include "rbk/misc/b64.h"
#include "sha.h"
#include <QDateTime>

std::string salt(size_t lenght, std::string) {
	return saltQS(lenght).toStdString();
}

QString salt(QString, size_t lenght) {
	return saltQS(lenght);
}

QString saltQS(size_t lenght) {
	auto ts = QDateTime::currentMSecsSinceEpoch() + rand();
	auto n  = QByteArray::number(ts);
	return sha1(n).left(lenght);
}
