#ifndef _HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_SERIALIZATION_QDATASTREAMER_H
#define _HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_SERIALIZATION_QDATASTREAMER_H

#include <QDataStream>
#include <string>

QDataStream& operator<<(QDataStream& out, const std::string& rhs);
QDataStream& operator>>(QDataStream& in, std::string& rhs);

#endif // _HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_SERIALIZATION_QDATASTREAMER_H
