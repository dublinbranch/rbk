#include "log.h"

Log::Log() {
	tsStart = QDateTime::currentDateTime();
	timer.start();
}

Log::Log(const QByteArray& _info, Category _category) {
	tsStart = QDateTime::currentDateTime();
	timer.start();
	this->stdOut   = _info;
	this->category = _category;
}

Log::Log(const std::exception& e, const char* func) {
	tsStart  = QDateTime::currentDateTime();
	stdErr   = e.what();
	category = Exception;
	section  = func;
}

void Log::setEnd() {
	elapsed = timer.nsecsElapsed();
}
