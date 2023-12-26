#include "jsonreader.h"
#include <cxxabi.h>
#include <memory>
#include <string>

using namespace rapidjson;

QByteArray JSONReader::subJsonRender(rapidjson::Value* el, bool pretty) {
	// https://github.com/Tencent/rapidjson/issues/1035

	StringBuffer buffer;
	if (pretty) {
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		el->Accept(writer);
		return {buffer.GetString(), static_cast<int>(buffer.GetSize())};
	} else {
		PrettyWriter<StringBuffer> writer(buffer);
		el->Accept(writer);
		return {buffer.GetString(), static_cast<int>(buffer.GetSize())};
	}

	// OLD
	//		auto&               allocator = json.GetAllocator();
	//		rapidjson::Document d;
	//		rapidjson::Value    v2(*el, allocator);

	//		rapidjson::StringBuffer                          buffer;
	//		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	//		d.CopyFrom(*el, allocator);
	//		d.Accept(writer);
	//		QByteArray res = "\n";
	//		res.append(buffer.GetString(), buffer.GetSize());
	//		res.append("\n");
	//		return res;
}

QByteArray JSONReader::jsonRender() {
	rapidjson::StringBuffer                          buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	json.Accept(writer);
	QByteArray res = "\n";
	res.append(buffer.GetString(), static_cast<int>(buffer.GetSize()));
	res.append("\n");
	return res;
}

bool JSONReader::stageClear(rapidjson::Value* el, bool verbose) {
	bool isEmpty = true;
	//check if we have nothing left (or all set as null, so we can delete)
	if (el->IsObject()) {
		for (auto&& iter : el->GetObject()) {
			isEmpty = isEmpty & stageClear(&iter.value, false);
			if (!isEmpty) {
				break;
			}
		}
	} else if (el->IsArray()) {
		for (auto&& iter : el->GetArray()) {
			isEmpty = isEmpty & stageClear(&iter, false);
			if (!isEmpty) {
				break;
			}
		}
	} else if (!el->IsNull()) {
		isEmpty = false;
	}

	if (isEmpty) {
		el->SetNull();
		return true;
	} else if (verbose) {
		qCritical().noquote() << "non empty local JSON block" << subJsonRender(el)
		                      << "Full JSON is now" << jsonRender()
		                      << QStacker16(4, QStackerOptLight);
	}
	return false;
}

bool JSONReader::stageClear() {
	bool isEmpty = true;
	for (auto&& iter : json.GetObject()) {
		isEmpty = isEmpty & stageClear(&iter.value, false);
		if (!isEmpty) {
			break;
		}
	}
	if (!isEmpty) {
		qCritical().noquote() << "non empty Full JSON at the end of parsing, remnant is" << jsonRender()
		                      << QStacker16(4, QStackerOptLight);
		return false;
	}
	return true;
}

bool JSONReader::parse(const std::string& raw) {
	return parse(raw.data());
}

bool JSONReader::parse(const QByteArray& raw) {
	return parse(raw.constData());
}

bool JSONReader::parse(const char* raw) {
	if (strlen(raw) < 2) {
		parseError = empty;
		return false;
	};
	rapidjson::StringStream                                 ss(raw);
	rapidjson::CursorStreamWrapper<rapidjson::StringStream> csw(ss);
	json.ParseStream(csw);
	if (json.HasParseError()) {
		qCritical().noquote() << QSL("Problem parsing json on line: %1 , pos: %2\nRaw Json: %3")
		                             .arg(csw.GetLine())
		                             .arg(csw.GetColumn())
		                             .arg(QString(raw).left(512))
		                      << QStacker16Light();
		parseError = invalid;
		return false;
	}
	return true;
}
