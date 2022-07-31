#pragma once

#include "rapidjson/includeMe.h"
#include "various.h"
#include <QByteArray>
#include <QString>

class rapidfunkz {
      public:
	//if not found, return a defaul value
	static const rapidjson::GenericValue<rapidjson::UTF8<>>& safeGet(const rapidjson::GenericValue<rapidjson::UTF8<>>& line, const char* name, const rapidjson::GenericValue<rapidjson::UTF8<>>& defaultVal);
	static const rapidjson::GenericValue<rapidjson::UTF8<>>  ZeroString;
	static const rapidjson::GenericValue<rapidjson::UTF8<>>  EmptyString;
	static const rapidjson::GenericValue<rapidjson::UTF8<>>  ZeroDouble;
	static const rapidjson::GenericValue<rapidjson::UTF8<>>  ZeroInt;
	//IS ARRAY IS complex and annoyng
};
/**
 * @brief jsonValue
 * use like 
 * auto& data = static_cast<JSafe&>(line["insights"]["data"].GetArray()[0]);
 * data.sfGet<double>("key");
 */
typedef rapidjson::GenericValue<rapidjson::UTF8<>> jsonValue;
class JSafe : public jsonValue {
      public:
	JSafe& operator=(const JSafe&) = delete;
	JSafe(const JSafe&)            = delete;

	static JSafe getDummy(){
		return JSafe();
	}

	template <typename V>
	V getta(const char* key) const {
		static_assert(std::is_arithmetic<V>::value, "This function can be used only for aritmetic types");
		auto iter = this->FindMember(key);
		if (iter != this->MemberEnd()) {
			const auto& value = iter->value;
			auto        type  = value.GetType();
			switch (type) {
			case rapidjson::Type::kNumberType:
				return value.Get<V>();
			case rapidjson::Type::kStringType: {
				bool   ok;
				double res;
				auto   qb = QByteArray::fromRawData(value.GetString(), value.GetStringLength());
				if constexpr (std::is_floating_point<V>::value) {
					res = qb.toDouble(&ok);
				} else if constexpr (std::is_signed<V>::value) {
					res = qb.toLongLong(&ok);
				} else {
					res = qb.toULongLong(&ok);
				}

				if (!ok) {
					throw QString("%1 is not an int, or directly convertible in an aritmetic type, it is a string %2").arg(key).arg(QString(qb));
				}
				return res;
			}
			default:
				throw QString("%1 is not an int, or directly convertible in an aritmetic type,, it is a %2").arg(key).arg(printType(type));
			}
		} else {
			return 0;
		}
	}
	template <typename V>
	bool getta(const char* key, V& value) const {
		auto iter = this->FindMember(key);
		if (iter == this->MemberEnd()) {
			return false;
		}
		value = getta<V>(key);
		return true;
	}
	template <typename V>
	static V getta(const jsonValue& json, const char* key) {
		return static_cast<const JSafe&>(json).getta<V>(key);
	}
	template <typename V>
	static bool getta(const jsonValue& json, const char* key, V& value) {
		auto iter = json.FindMember(key);
		if (iter == json.MemberEnd()) {
			return false;
		}
		value = JSafe::getta<V>(json, key);
		return true;
	}
private:
	JSafe();
	bool dummy = false;
};
