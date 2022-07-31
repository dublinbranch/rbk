Either include with

include(rapidjson/rapidjson.pri)


Or

Remember to define somewhere the

#include "rapidjson/includeMe.h"
#include <QDebug>
#include "QStacker/qstacker.h"

void rapidAssert(bool condition) {
	if (!condition && rapidAssertEnabled) {
		if(rapidAssertPrintTrace){
			qCritical().noquote() << QStacker();
		}
		throw 3;
	}
}


Also add

QMAKE_CXXFLAGS += -msse4.2

