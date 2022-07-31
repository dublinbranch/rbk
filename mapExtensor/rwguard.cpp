#include "rwguard.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/QStacker/qstacker.h"
#include <QDebug>
#include <QString>

#define QSL(str) QStringLiteral(str)

RWGuard::RWGuard(std::shared_mutex* newMutex) {
	mutex = newMutex;
}

RWGuard::~RWGuard() {
	unlock(false);
}

void RWGuard::setMutex(std::shared_mutex* newMutex) {
	mutex = newMutex;
}

void RWGuard::lock() {
	mutex->lock();
	exclusive = true;
}

void RWGuard::lockExclusive() {
	lock();
}

void RWGuard::lockShared() {
	mutex->lock_shared();
	shared = true;
}

void RWGuard::unlock(bool warnOnUnlockError) {
	if (shared) {
		mutex->unlock_shared();
		shared = false;
	} else if (exclusive) {
		mutex->unlock();
		exclusive = false;
	} else {
		if (warnOnUnlockError) {
			qCritical() << QSL("You can not UNlock a NON locked mutex...") << QStacker16Light();
		}
	}
}
