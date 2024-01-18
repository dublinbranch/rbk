#include "log.h"
#include "rbk/BoostJson/extra.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/fmtExtra/includeMe.h"

namespace bj = boost::json;

std::string Log::serialize() {
	used = true;
	return pretty_print(toJson());
}

boost::json::object Log::toJson() {
	bj::object obj;
	obj["category"]   = category;
	obj["section"]    = section.toStdString();
	obj["tsStart"]    = tsStart.toString(mysqlDateMicroTimeFormat).toStdString();
	obj["elapsed"]    = elapsed;
	obj["stdOut"]     = stdOut.toStdString();
	obj["stdErr"]     = stdErr.toStdString();
	obj["stackTrace"] = stackTrace.toStdString();
	bj::array arr;
	for (auto& log : subLogs) {
		arr.push_back(log.toJson());
	}
	obj["subLogs"] = arr;
	used           = true;
	return obj;
}

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

Log::~Log() {
	if (!used) {
		qCritical().noquote() << "Log got wasted, this is not what you want...! use me" << QStacker16Light();
	}
}

void Log::push(Log&& log) {
	log.used = true;
	subLogs.emplace_back(std::move(log));
}

void Log::push(Log& log) {
	log.used = true;
	subLogs.emplace_back(std::move(log));
}

void Log::setEnd() {
	elapsed = timer.nsecsElapsed();
}
