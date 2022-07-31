#include "safeGet.h"

rapidjson::GenericValue<rapidjson::UTF8<>> zeroStringInit() {
	rapidjson::GenericValue<rapidjson::UTF8<>> v;
	v.SetString("0");
	return v;
}

rapidjson::GenericValue<rapidjson::UTF8<>> emptyStringInit() {
	rapidjson::GenericValue<rapidjson::UTF8<>> v;
	v.SetString("");
	return v;
}

rapidjson::GenericValue<rapidjson::UTF8<>> zeroDoubleInit() {
	rapidjson::GenericValue<rapidjson::UTF8<>> v;
	v.SetDouble(0);
	return v;
}
rapidjson::GenericValue<rapidjson::UTF8<>> zeroIntInit() {
	rapidjson::GenericValue<rapidjson::UTF8<>> v;
	v.SetInt64(0);
	return v;
}

const rapidjson::GenericValue<rapidjson::UTF8<>> rapidfunkz::ZeroString  = zeroStringInit();
const rapidjson::GenericValue<rapidjson::UTF8<>> rapidfunkz::EmptyString = emptyStringInit();
const rapidjson::GenericValue<rapidjson::UTF8<>> rapidfunkz::ZeroDouble  = zeroDoubleInit();
const rapidjson::GenericValue<rapidjson::UTF8<>> rapidfunkz::ZeroInt     = zeroIntInit();

const rapidjson::GenericValue<rapidjson::UTF8<>>& rapidfunkz::safeGet(const rapidjson::GenericValue<rapidjson::UTF8<>>& line, const char* name, const rapidjson::GenericValue<rapidjson::UTF8<>>& defaultVal) {
	if (!line.IsObject()) {
		return defaultVal;
	}
	auto iter = line.FindMember(name);
	if (iter == line.MemberEnd()) {
		return defaultVal;
	}
	return iter->value;
}

//SMART_ENUM(inner, uint) {
//	SM_ENUM_ELEM(kBoolFlag, 0x0008);
//	SM_ENUM_ELEM(kNumberFlag, 0x0010);
//	SM_ENUM_ELEM(kIntFlag, 0x0020);
//	SM_ENUM_ELEM(kUintFlag, 0x0040);
//	SM_ENUM_ELEM(kInt64Flag, 0x0080);
//	SM_ENUM_ELEM(kUint64Flag, 0x0100);
//	SM_ENUM_ELEM(kDoubleFlag, 0x0200);
//	SM_ENUM_ELEM(kStringFlag, 0x0400);
//	SM_ENUM_ELEM(kCopyFlag, 0x0800);
//	SM_ENUM_ELEM(kInlineStrFlag, 0x1000);
//};

//const QString check0(uint v) {
//	QStringList buf;
//	for (auto&& scan : inner::values()) {
//		if (v & scan.to_integral()) {
//			buf.append(scan.to_string());
//		}
//	}
//	return buf.join(",");
//}

//auto     type2 = check0(v.getFlag());
/* add in document.h line 1731
	const uint16_t getFlag() const {
		return data_.f.flags;
	}
	*/

JSafe::JSafe(){
	dummy = true;
}
