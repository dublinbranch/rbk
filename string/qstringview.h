#ifndef HOME_ROY_PUBLIC_RBK_STRING_QSTRINGVIEW_H
#define HOME_ROY_PUBLIC_RBK_STRING_QSTRINGVIEW_H

class QString;
class QStringView;

//just why midRef no longer exists... bha
QStringView midView(const QString& string, int pos, int len = -1);

#endif // HOME_ROY_PUBLIC_RBK_STRING_QSTRINGVIEW_H
