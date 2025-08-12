#pragma once

#include "rbk/misc/swapType.h"
#include "rbk/rapidjson/includeMe.h"
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

	static JSafe getDummy() {
		return {};
	}

	template <typename V>
	V getta(const char* key) const {
		V    res;
		auto iter = this->FindMember(key);
		if (iter != this->MemberEnd()) {
			const auto& value = iter->value;
			auto        type  = value.GetType();
			if constexpr (std::is_arithmetic_v<V>) {
				if (type == rapidjson::Type::kNumberType) {
					return value.Get<V>();
				}
			}
			if (type == rapidjson::Type::kStringType) {
				auto qb = QByteArray::fromRawData(value.GetString(), (int)value.GetStringLength());
				swapType(qb, res);
				return res;
			}

			throw QString("%1 is not an int, or directly convertible in an aritmetic type,, it is a %2").arg(key, printType(type));
		}
		return {};
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
};
