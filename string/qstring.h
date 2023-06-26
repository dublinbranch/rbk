#ifndef _HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_STRING_QSTRING_H
#define _HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_STRING_QSTRING_H

#include <QString>
#include <string>

QString    Q16(std::string s);
QByteArray Q8(std::string s);

//also remove newline and tab
[[nodiscard]] QString simplifyMore(const QString& original);

[[nodiscard]] QByteArray simplifyMore(const QByteArray& original);

#endif
