#ifndef CRC_H
#define CRC_H

#include "rbk/misc/intTypes.h"

class QString;
class QByteArray;
u64 crc32(const QString& val);
u64 crc32(const QByteArray& val);

#endif // CRC_H
