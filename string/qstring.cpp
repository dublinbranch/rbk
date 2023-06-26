#include "qstring.h"

QString Q16(std::string s) {
	return QString::fromStdString(s);
}

QByteArray Q8(std::string s) {
	return QByteArray::fromStdString(s);
}

QString simplifyMore(const QString& original) {
	QString cleaned;

	auto c1 = original.simplified();

	cleaned.reserve(c1.size());

	for (auto& qc : c1) {
		switch (qc.toLatin1()) {
		case '\n':
		case '\t':
		case '\r':
			continue;
		default:
			cleaned.append(qc);
		}
	}
	return cleaned;
}

QByteArray simplifyMore(const QByteArray& original) {
	QByteArray cleaned;

	auto c1 = original.simplified();

	cleaned.reserve(c1.size());

	for (auto& qc : c1) {
		switch (qc) {
		case '\n':
		case '\t':
		case '\r':
			continue;
		default:
			cleaned.append(qc);
		}
	}
	return cleaned;
}
