#pragma once

#include "rapidjson/includeMe.h"
#include <QString>
#include <memory>

QString printType(rapidjson::Type t);

struct JsonDecoder {
	std::shared_ptr<rapidjson::Document> json = nullptr;

	QString    msg;
	QByteArray raw;
	size_t     line   = 0;
	size_t     column = 0;
	bool       valid  = false;
};
JsonDecoder parse(const QByteArray& raw, bool quiet = false);
