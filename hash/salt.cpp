#include "salt.h"
#include "rbk/QStacker/exceptionv2.h"
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
	if (lenght > 86) {
		throw ExceptionV2("max salt lenght atm is 86, edit the code if you want more");
	}
	auto ts  = QDateTime::currentMSecsSinceEpoch() + rand();
	auto n   = QByteArray::number(ts);
	auto sha = sha512(n, false);
	//for *REASON* crypto is retarded, and is not using the standard base64 but they replace 1 character -.- the + with the .
	return sha1(n).replace("+", ".").left(lenght);
}
