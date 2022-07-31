#include "various.h"
#include "rapidjson/cursorstreamwrapper.h"
#include "rapidjson/includeMe.h"
#include "rapidjson/pointer.h"
#include "rapidjson/prettywriter.h"
#include <QDebug>
#define QSL(str) QStringLiteral(str)

using namespace rapidjson;
using namespace std;

QString printType(rapidjson::Type t) {
	switch (t) {
	case Type::kNullType:
		return "null";
		break;
	case Type::kFalseType:
		return "bool (false)";
		break;
	case Type::kTrueType:
		return "bool (true)";
		break;
	case Type::kObjectType:
		return "object";
		break;
	case Type::kArrayType:
		return "array";
		break;
	case Type::kStringType:
		return "string";
		break;
	case Type::kNumberType:
		return "number";
		break;
	}
	return QString("unsupported type");
}

JsonDecoder parse(const QByteArray& raw, bool quiet) {
	JsonDecoder res;
	res.raw  = raw;
	res.json = std::make_shared<rapidjson::Document>();
	rapidjson::StringStream                                 ss(raw.constData());
	rapidjson::CursorStreamWrapper<rapidjson::StringStream> csw(ss);
	res.json->ParseStream(csw);
	if (res.json->HasParseError()) {
		res.valid     = false;
		res.column    = csw.GetColumn();
		res.line      = csw.GetLine();
		auto jsonPart = QString(raw);
		jsonPart.truncate(2048);
		res.msg = QSL("Problem parsing json on line: %1 , pos: %2 \n Json was like %3").arg(csw.GetLine()).arg(csw.GetColumn()).arg(jsonPart);
		if (!quiet) {
			throw res.msg;
		}
	} else {
		res.valid = true;
	}
	return res;
}
