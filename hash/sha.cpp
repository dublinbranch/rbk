#include "sha.h"
#include <QCryptographicHash>

QByteArray sha512(const QByteViewV2& original, bool urlSafe) {
	auto sha = QCryptographicHash::hash(original, QCryptographicHash::Algorithm::Sha512);
	if (urlSafe) {
		return sha.toBase64(QByteArray::Base64Option::Base64UrlEncoding | QByteArray::Base64Option::OmitTrailingEquals);
	}
	return sha.toBase64(QByteArray::Base64Option::OmitTrailingEquals);
}

QByteArray sha256(const QByteViewV2& original, bool urlSafe) {
	auto sha = QCryptographicHash::hash(original, QCryptographicHash::Algorithm::Sha256);
	if (urlSafe) {
		return sha.toBase64(QByteArray::Base64Option::Base64UrlEncoding | QByteArray::Base64Option::OmitTrailingEquals);
	}
	return sha.toBase64(QByteArray::Base64Option::OmitTrailingEquals);
}

QByteArray sha1(const QByteArray& original, bool urlSafe) {
	auto sha = QCryptographicHash::hash(original, QCryptographicHash::Algorithm::Sha1);
	if (urlSafe) {
		return sha.toBase64(QByteArray::Base64Option::Base64UrlEncoding | QByteArray::Base64Option::OmitTrailingEquals);
	} else {
		return sha.toBase64(QByteArray::Base64Option::OmitTrailingEquals);
	}
}
QByteArray sha1(const QString& original, bool urlSafe) {
	return sha1(original.toUtf8(), urlSafe);
}
QString sha1QS(const QString& original, bool urlSafe) {
	return sha1(original, urlSafe);
}

QByteArray sha1(const std::string& original, bool urlSafe) {
	return sha1(QByteArray::fromStdString(original), urlSafe);
}
