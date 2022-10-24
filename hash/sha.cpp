#include "sha.h"
#include <QCryptographicHash>

QByteArray sha512(const QByteArray& original, bool urlSafe) {
	auto sha1 = QCryptographicHash::hash(original, QCryptographicHash::Algorithm::Sha512);
	if (urlSafe) {
		return sha1.toBase64(QByteArray::Base64Option::Base64UrlEncoding | QByteArray::Base64Option::OmitTrailingEquals);
	}
	return sha1;
}

QByteArray sha1(const QByteArray& original, bool urlSafe) {
	auto sha1 = QCryptographicHash::hash(original, QCryptographicHash::Algorithm::Sha1);
	if (urlSafe) {
		return sha1.toBase64(QByteArray::Base64Option::Base64UrlEncoding | QByteArray::Base64Option::OmitTrailingEquals);
	}
	return sha1;
}
QByteArray sha1(const QString& original, bool urlSafe) {
	return sha1(original.toUtf8(), urlSafe);
}
QString sha1QS(const QString& original, bool urlSafe) {
	return sha1(original, urlSafe);
}
