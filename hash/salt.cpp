#include "salt.h"
#include "rbk/misc/b64.h"
#include "sha.h"
#include <QDateTime>

std::string salt(std::string, int lenght) {
	return saltQS(lenght).toStdString();
}

QString salt(QString, int lenght) {
	return saltQS(lenght);
}

QString saltQS(int lenght) {
	auto ts = QDateTime::currentMSecsSinceEpoch() + rand();
	auto n  = QByteArray::number(ts);
	return sha1(n).left(lenght);
}
