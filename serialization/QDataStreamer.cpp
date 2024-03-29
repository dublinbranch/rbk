#include "QDataStreamer.h"
#include <QByteArray>

QDataStream& operator<<(QDataStream& out, const std::string& rhs) {
	QByteArray raw;
	raw.setRawData(rhs.data(), static_cast<uint>(rhs.size()));
	out << raw;
	return out;
}

QDataStream& operator>>(QDataStream& in, std::string& rhs) {
	QByteArray raw;
	in >> raw;
	rhs = raw.toStdString();
	return in;
}
