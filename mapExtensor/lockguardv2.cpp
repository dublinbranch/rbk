#include "lockguardv2.h"
#include "rbk/QStacker/exceptionv2.h"
#include <QString>

#define QSL(str) QStringLiteral(str)

LockGuardV2::LockGuardV2(std::mutex* newMutex, bool lockNow) {
	mutex = newMutex;
	if (lockNow) {
		lock();
	}
}

LockGuardV2::~LockGuardV2() {
	if (didWeLockedIt) {
		mutex->unlock();
	}
}

void LockGuardV2::setMutex(std::mutex* newMutex) {
	mutex = newMutex;
}

void LockGuardV2::lock() {
	if (didWeLockedIt) {
		throw ExceptionV2(QSL("You can not lock a lock twice..."));
	}
	mutex->lock();
	didWeLockedIt = true;
}

bool LockGuardV2::tryLock() {
	if (didWeLockedIt) {
		throw ExceptionV2(QSL("You can not lock a lock twice..."));
	}
	if (mutex->try_lock()) {
		didWeLockedIt = true;
		return true;
	}
	return false;
}

void LockGuardV2::unlock() {
	if (didWeLockedIt) {
		mutex->unlock();
		didWeLockedIt = false;
	} else {
		throw ExceptionV2(QSL("You can not UNlock a NON locked mutex..."));
	}
}
