#ifndef CRC_H
#define CRC_H

class QString;
class QByteArray;
unsigned int crc32(const QString& val);
unsigned int crc32(const QByteArray& val);

#endif // CRC_H
