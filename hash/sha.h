#ifndef SHA_H
#define SHA_H

#include "rbk/string/stringoso.h"
#include <QByteArray>
#include <QString>

QByteArray sha512(const QByteAdt& original, bool urlSafe = true);

QByteArray sha256(const QByteAdt& original, bool urlSafe = true);

QByteArray sha1(const QByteArray& original, bool urlSafe = true);
QByteArray sha1(const QString& original, bool urlSafe = true);
QByteArray sha1(const std::string& original, bool urlSafe = true);
QString    sha1QS(const QString& original, bool urlSafe = true);

#endif // SHA_H
