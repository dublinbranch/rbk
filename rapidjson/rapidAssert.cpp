#include "rbk/QStacker/qstacker.h"
#include "includeMe.h"
#include <QDebug>
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/RAII/resetAfterUse.h"

void rapidAssert(bool condition) {
	if (!condition && rapidAssertEnabled) {
		if (rapidAssertPrintTrace) {
			//no reason to double print
			ResetOnExit r(cxaNoStack,true);
			qCritical().noquote() << QStacker16();
		}
		throw ExceptionV2("json error");
	}
}
