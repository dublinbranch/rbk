#include "log.h"
#include "rbk/BoostJson/extra.h"
#include "rbk/QStacker/qstacker.h"
#include "rbk/fmtExtra/includeMe.h"
#include <QDebug>

namespace bj = boost::json;

bool Log::hasError(bool recursive) const {
	if (recursive) {
		for (auto& log : subLogs) {
			if (log.hasError(true)) {
				return true;
			}
		}
	}
	return !stdErr.isEmpty();
}

std::string Log::serialize() {
	used = true;
	return pretty_print(toJson());
}

QString Log::serialize(QString) {
	return QString::fromStdString(serialize());
}

boost::json::object Log::toJson() {
	bj::object obj;
	obj["category"]   = category;
	obj["section"]    = section.toStdString();
	obj["options"]    = options;
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
		auto a = subLogs.empty();
		auto b = stdErr.isEmpty();
		auto c = stdOut.isEmpty();

		//if log has never been used and contain nothing... no problem
		if (a && b && c) {
			return;
		}
		qCritical().noquote() << "Log got wasted, this is not what you want...! use me" << QStacker16Light();
	}
}

void Log::push(Log&& log) {
	log.used = true;
	if (log.elapsed == 0) {
		log.setEnd();
	}
	subLogs.emplace_back(std::move(log));
}

void Log::push(Log& log) {
	log.used = true;
	if (log.elapsed == 0) {
		log.setEnd();
	}
	subLogs.emplace_back(std::move(log));
}

void Log::setEnd() {
	elapsed = timer.nsecsElapsed();
}
