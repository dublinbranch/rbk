#ifndef STRINGDEFINE_H
#define STRINGDEFINE_H

#include <QString>

#define QBL(str) QByteArrayLiteral(str)
#define QSL(str) QStringLiteral(str)

//Those variable are shared in many places, order of initialization is important!
//Inline will avoid to have multiple copy, and enforces having a single one
inline const QString     mysqlDateFormat          = "yyyy-MM-dd";                 //2022-12-05
inline const QString     mysqlDateTimeFormat      = "yyyy-MM-dd HH:mm:ss";        //2022-12-05 00:00:01
inline const QString     mysqlDateMicroTimeFormat = "yyyy-MM-dd HH:mm:ss.zzzzzz"; //2022-12-05 00:00:01.123456
inline const QString     fileDateTimeFormat       = "yyyy-MM-dd_HH:mm:ss";
inline const QString     SQLTableDateFormat       = "yyyy_MM_dd";
inline const QString     SQL_NULL                 = "NULL";
inline const QByteArray  BSQL_NULL                = "NULL";
inline const QByteArray  BZero                    = "0";
inline const QByteArray  BEmpty;
inline const QString     Zero       = "0";
inline const std::string S_SQL_NULL = "NULL";

#endif // STRINGDEFINE_H
