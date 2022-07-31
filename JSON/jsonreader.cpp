#include "jsonreader.h"
#include <cxxabi.h>
#include <memory>
#include <string>

using namespace rapidjson;
QString demangle(const char* name) {
	//Just copy pasted from stack overflow, I have no exact idea how this works
	int status = -4; // some arbitrary value to eliminate the compiler warning

	QString res = abi::__cxa_demangle(name, NULL, NULL, &status);

	if (status == 0) {
		return res;
	} else {
		//something did not worked, sorry
		return name;
	}
}

QByteArray JSONReader::subJsonRender(rapidjson::Value* el) {
	// https://github.com/Tencent/rapidjson/issues/1035

	StringBuffer               sb;
	PrettyWriter<StringBuffer> writer(sb);
	el->Accept(writer);
	return sb.GetString();

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
	res.append(buffer.GetString(), buffer.GetSize());
	res.append("\n");
	return res;
}

bool JSONReader::stageClear(rapidjson::Value* el, bool verbose) {
	bool empty = true;
	//check if we have nothing left (or all set as null, so we can delete)
	if (el->IsObject()) {
		for (auto&& iter : el->GetObject()) {
			empty = empty & stageClear(&iter.value, false);
			if (!empty) {
				break;
			}
		}
	} else if (el->IsArray()) {
		for (auto&& iter : el->GetArray()) {
			empty = empty & stageClear(&iter, false);
			if (!empty) {
				break;
			}
		}
	} else if (!el->IsNull()) {
		empty = false;
	}

	if (empty) {
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
	bool empty = true;
	for (auto&& iter : json.GetObject()) {
		empty = empty & stageClear(&iter.value, false);
		if (!empty) {
			break;
		}
	}
	if (!empty) {
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
	rapidjson::StringStream                                 ss(raw);
	rapidjson::CursorStreamWrapper<rapidjson::StringStream> csw(ss);
	json.ParseStream(csw);
	if (json.HasParseError()) {
		qCritical().noquote() << QSL("Problem parsing json on line: %1 , pos: %2\nRaw Json: %3")
		                             .arg(csw.GetLine())
		                             .arg(csw.GetColumn())
		                             .arg(QString(raw).left(512))
		                      << QStacker16Light();
		return false;
	}
	return true;
}
