#ifndef B64_H
#define B64_H

#include <QString>

QString    base64this(const char* param);
QString    toBase64(const QString& url, bool urlSafe = true);
QByteArray fromBase64(const QByteArray& url64, bool urlSafe = true);
QString    fromBase64(const QString& url64, bool urlSafe = true);
//bool       isB64Valid(QString input, bool checkLength = false);
bool    isB64Valid(const QByteArray& input, bool urlSafe = false);
bool    isB64Valid(const QString& input, bool urlSafe = false);
QString base64this(const QByteArray& param);
QString base64this(const QString& param);
QString base64this(const std::string& param);
QString base64this(const std::string_view& param);
QString mayBeBase64(const QString& original, bool emptyAsNull = false);
QString base64Nullable(const QString& param, bool emptyAsNull = false);
QString base64Nullable(const QString* param, bool emptyAsNull = false);
QString base64Nullable4Where(const QString& param, bool emptyAsNull = false);

QByteArray shortMd5(const QByteArray& string, bool hex);
QByteArray shortMd5(const QString& string, bool hex);
#endif // B64_H
