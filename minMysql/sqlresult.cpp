#include "sqlresult.h"

QString sqlResult::toString() {
	QString s;
	for (sqlRow& row : *this) {
		s += asString(row) + "\n";
	}
	return s;
}

const mapV2<QByteArray, MyType> &sqlResult::getTypes() const {
	if (types.empty()) {
		throw ExceptionV2("no type info loaded, verify how is this result set created");
	}
	return types;
}
