#include "QCommandLineParserV2.h"
#include "rbk/QStacker/exceptionv2.h"

QString QCommandLineParserV2::require(const QString& key, const char* msg) {
	//do not use isSet, else default param will fail
	if (value(key).isEmpty()) {
		if (msg) {
			throw ExceptionV2(msg);
		} else {
			throw ExceptionV2(QSL("missing required parameter %1").arg(key),6);
		}
	} else {
		return value(key);
	}
}
