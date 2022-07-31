#include "crc.h"
#include <QByteArray>
#include <QString>
#include <boost/crc.hpp>

uint crc32(const QString& val) {
	return crc32(val.toUtf8());
}

unsigned int crc32(const QByteArray& val) {
	boost::crc_32_type c;
	c.process_bytes(val.data(), val.size());
	auto cc = c.checksum();
	return cc;
}
