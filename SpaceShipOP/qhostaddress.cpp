#include "qhostaddress.h"
#include <QHostAddress>

std::strong_ordering operator<=>(const QHostAddress& lhs, const QHostAddress& rhs) {
	return lhs.toIPv4Address() <=> rhs.toIPv4Address();
}
