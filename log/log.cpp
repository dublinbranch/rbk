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
	if (category == notSet) {
		throw ExceptionV2(F("processing a log with unset category, is that a good or bad one ?!? Section is {}", section));
	}
	obj["category"] = category;

	if (section.isEmpty()) {
		throw ExceptionV2("processing a log with not section, this is useless, fix the logic!");
	}
	obj["section"] = section.toStdString();
	if (!options.empty()) {
		obj["options"] = options;
	}

	obj["tsStart"] = tsStart.toString(mysqlDateMicroTimeFormat).toStdString();
	obj["elapsed"] = elapsed;

	if (!stdOut.isEmpty()) {
		obj["stdOut"] = stdOut.toStdString();
	}

	if (!stdErr.isEmpty()) {
		obj["stdErr"] = stdErr.toStdString();
	}

	if (stackTrace.isEmpty()) {
		throw ExceptionV2(F("processing a log with no stacktrace, put here something! section is {}", section));
	}
	obj["stackTrace"] = stackTrace.toStdString();

	if (subLogs.empty()) {
		bj::array arr;
		for (auto& log : subLogs) {
			arr.push_back(log.toJson());
		}
		obj["subLogs"] = arr;
	}

	used = true;
	return obj;
}

boost::json::object Log::toJson4panel() const {
	bj::object json;
	json["status"]  = "error";
	json["message"] = stdErr;
	return json;
}

Log::Log() {
	stackTrace = QStacker16Light(6).toUtf8();
	tsStart    = QDateTime::currentDateTime();
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
