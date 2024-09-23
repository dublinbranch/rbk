#include "crc.h"
#include <QByteArray>
#include <QString>
#include <boost/crc.hpp>

u64 crc32(const QString& val) {
	return crc32(val.toUtf8());
}

u64 crc32(const QByteArray& val) {
	boost::crc_32_type c;
    c.process_bytes(val.data(), (std::size_t)val.size());
	auto cc = c.checksum();
	return cc;
}
