#include "threadstatush.h"

ThreadStatus threadStatus;

//Just a default for the program who do not uses this technology
static thread_local ThreadStatus::Status localThreadStatusValue;
thread_local ThreadStatus::Status*       localThreadStatus = &localThreadStatusValue;

void ThreadStatus::Timing::addSqlTime(qint64 addMe) {
	if (flush) {
		sqlDeferred += addMe;
	} else {
		sqlImmediate += addMe;
	}
}

void ThreadStatus::Timing::addCurlTime(qint64 addMe) {
	if (flush) {
		curlDeferred += addMe;
	} else {
		curlImmediate += addMe;
	}
}

qint64 ThreadStatus::Timing::total() const {
	return timer.nsecsElapsed();
}

qint64 ThreadStatus::Timing::execution() const {
	return total() - (curlDeferred + curlImmediate + sqlDeferred + sqlImmediate + IO.nsecsElapsed() + clickHouse.nsecsElapsed());
}

void ThreadStatus::Timing::reset() {
	*this = Timing();
	timer.start();
}

void ElapsedTimerV2::start() {
	paused = false;
	timer.restart();
}

qint64 ElapsedTimerV2::pause() {
	paused = true;
	if (!timer.isValid()) {
		throw ExceptionV2("invalid timer ? there must be some logic bug somewhere");
	}
	total += timer.nsecsElapsed();
	return total;
}

qint64 ElapsedTimerV2::nsecsElapsed() const {
	if (paused) {
		return total;
	}
	return total + timer.nsecsElapsed();
}
