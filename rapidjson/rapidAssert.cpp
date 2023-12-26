#include "rbk/QStacker/qstacker.h"
#include "includeMe.h"
#include <QDebug>
#include "rbk/QStacker/exceptionv2.h"

void rapidAssert(bool condition) {
	if (!condition && rapidAssertEnabled) {
		if (rapidAssertPrintTrace) {
			//no reason to double print
			cxaNoStack = true;
			qCritical().noquote() << QStacker16();
		}
		throw ExceptionV2("json error");
	}
}
