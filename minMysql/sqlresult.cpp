#include "sqlresult.h"


QString sqlResult::toString() {
	QString s;
	for (sqlRow& row : *this) {
		s += asString(row) + "\n";
	}
	return s;
}
