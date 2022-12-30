#include "QDataStreamer.h"

QDataStream& operator<<(QDataStream& out, const std::string& rhs) {
	QByteArray raw;
	raw.setRawData(rhs.data(), rhs.size());
	out << raw;
	return out;
}

QDataStream& operator>>(QDataStream& in, std::string& rhs) {
	QByteArray raw;
	in >> raw;
	rhs = raw.toStdString();
	return in;
}
