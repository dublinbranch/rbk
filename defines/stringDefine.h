#ifndef STRINGDEFINE_H
#define STRINGDEFINE_H

#include <QString>

#define QBL(str) QByteArrayLiteral(str)
#define QSL(str) QStringLiteral(str)

//Those variable are shared in many places, order of initialization is important!
//Inline will avoid to have multiple copy, and enforces having a single one
inline const QString    mysqlDateFormat     = "yyyy-MM-dd";          //2022-12-05
inline const QString    mysqlDateTimeFormat = "yyyy-MM-dd HH:mm:ss"; //2022-12-05 00:00:01
inline const QString    fileDateTimeFormat  = "yyyy-MM-dd_HH:mm:ss";
inline const QString    SQL_NULL            = "NULL";
inline const QByteArray BSQL_NULL           = "NULL";
inline const QByteArray BZero               = "0";
inline const QString    Zero                = "0";

#endif // STRINGDEFINE_H
