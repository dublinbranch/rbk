#pragma once
#include "rbk/rapidjson/includeMe.h"

#include "rbk/rapidjson/cursorstreamwrapper.h"
#include "rbk/rapidjson/pointer.h"
#include "rbk/rapidjson/prettywriter.h"

#include "rbk/JSON/various.h"
#include "JSONReaderConst.h"
#include "rbk/QStacker/qstacker.h"

#include "rbk/magicEnum/magic_from_string.hpp"
#include <QDebug>
#include <QString>

#define QBL(str) QByteArrayLiteral(str)
#define QSL(str) QStringLiteral(str)

QString demangle(const char* name);

template <typename Type>
QString getTypeName() {
	auto name = typeid(Type).name();
	return demangle(name);
}

class JSONReader {
      public:
	rapidjson::Document json;
	//No idea how to have this value flow around
	rapidjson::Type mismatchedType;
	//keep the value during the swap process
	bool keep = false;

	template <typename Type>
	/**
	 * @brief swapperJPtr
	 * @param key
	 * @param value
	 */
	void swapperJPtr(const char* JSONPointer, Type& value, rapidjson::Value* src = nullptr) {
		rapidjson::Value* refJson = src;

		if (src) {
			//apply json ptr to a subblock!
			refJson = src;
		} else {
			refJson = &json;
		}

		rapidjson::Value* useMe = rapidjson::Pointer(JSONPointer).Get(*refJson);
		switch (swapperInner(useMe, value)) {
		case SwapRes::errorMissing:
			qCritical().noquote() << "missing JSON Path:" << JSONPointer << "for REQUIRED parameter" + QStacker16(4, QStackerOptLight);
			exit(1);
			break;
		case SwapRes::swapped:
			rapidjson::Pointer(JSONPointer).Erase(*refJson);
			break;
		case SwapRes::typeMismatch:
			qCritical().noquote() << QSL("Type mismatch! Expecting %1 found %2 for %3")
			                                 .arg(getTypeName<Type>())
			                                 .arg(printType(mismatchedType))
			                                 .arg(JSONPointer) +
			                             QStacker16Light();
			exit(1);
			break;
		case SwapRes::notFound:
			//nothing to do here
			break;
		}
	}

	template <typename Type>
	void swapper(rapidjson::Value::MemberIterator& iter, const char* key, Type& value) {
		return swapper(&iter->value, key, value);
	}

	template <typename Type>
	void swapper(rapidjson::Value& obj, const char* key, Type& value) {
		swapper(&obj, key, value);
	}

	template <typename Type>
	Type swapper(rapidjson::Value* obj, const char* key) {
		Type value;
		swapper(obj, key, value);
		return value;
	}

	template <typename Type>
	void swapper(rapidjson::Value* obj, const char* key, Type& value) {
		if (!obj) {
			qCritical().noquote() << "You need to check for branch existance before this point (we extract the leaf!)" << QStacker16Light();
			exit(1);
		}
		if (!obj->IsObject()) {
			qCritical().noquote() << QSL("You are searching %1 inside this element, but this is not an object, is a %2").arg(key).arg(obj->GetType()) << QStacker16Light();
			exit(1);
		}

		auto              iter  = obj->FindMember(key);
		rapidjson::Value* useMe = nullptr;
		if (iter != obj->MemberEnd()) {
			useMe = &iter->value;
		}

		auto swapRes = swapperInner(useMe, value);
		switch (swapRes) {
		case SwapRes::errorMissing:
			qCritical().noquote() << "missing KEY:" << key << "for REQUIRED parameter" + QStacker16Light();
			exit(1);
			break;
		case SwapRes::swapped:
			if (!keep) {
				obj->RemoveMember(iter);
			}
			break;
		case SwapRes::typeMismatch:
			qCritical().noquote() << QSL("Type mismatch! Expecting %1 found %2 for %3")
			                                 .arg(getTypeName<Type>())
			                                 .arg(printType(mismatchedType))
			                                 .arg(key) +
			                             QStacker16Light();
			exit(1);
			break;
		case SwapRes::notFound:
			//nothing to do here
			break;
		}
	}

	template <typename Type>
	void swapper(rapidjson::Value* obj, Type& value) {
		switch (swapperInner(obj, value)) {
		case SwapRes::swapped:
			if (!keep) {
				obj->SetNull();
			}
			break;
		case SwapRes::typeMismatch:
			qCritical().noquote() << QSL("Type mismatch! Expecting %1 found %2 for %3")
			                                 .arg(getTypeName<Type>())
			                                 .arg(printType(mismatchedType)) +
			                             QStacker16Light();
			exit(1);
		case SwapRes::errorMissing:
			qCritical().noquote() << "missing value for REQUIRED parameter" + QStacker16Light();
			exit(1);
			break;
		case SwapRes::notFound:
			//nothing to do here
			break;
		}
	}

	template <typename Type>
	Type swapGet(rapidjson::Value* obj) {
		Type temp;
		swapper(obj, temp);
		return temp;
	}

	enum SwapRes {
		typeMismatch,
		errorMissing, //the value was needed, but not founded
		swapped,      //all ok
		notFound      // missing but not required
	};

	template <typename Type>
	void unroll(rapidjson::Value* el, Type vector) {
		vector.clear();

		auto array = el->GetArray();

		for (auto iter = array.begin(); iter != array.end();) {
			typename Type::value_type value;
			auto                      res = swapperInner(iter, value);
			switch (res) {
			case SwapRes::typeMismatch:
				qCritical().noquote() << QSL("Type mismatch! Expecting %1 found %2 for %3")
				                                 .arg(getTypeName<Type>())
				                                 .arg(printType(mismatchedType)) +
				                             QStacker16Light();
				exit(1);
			case SwapRes::swapped:
				vector.push_back(value);
				break;
			default:
				qCritical().noquote() << QSL("This should not happen: %1").arg(res) +
				                             QStacker16Light();
				exit(1);
			}
			iter = array.Erase(iter);
		}
		stageClear(el);
	}

      private:
	template <typename Type>
	SwapRes swapperInner(rapidjson::Value* obj, Type& value) {
		if (obj == nullptr) {
			//check if the default value is a sensible one
			//empty is a legit value, that is why we use a canary one `SET_ME`
			if constexpr (std::is_same<Type, QByteArray>::value) {
				if (value == JSONReaderConst::setMe8) {
					return SwapRes::errorMissing;
				}
			} else if constexpr (std::is_same<Type, QString>::value) {
				if (value == JSONReaderConst::setMe) {
					return SwapRes::errorMissing;
				}
				//higher precedence because, is_integral bool == true!
			} else if constexpr (std::is_same<Type, bool>::value) {
				//no reason ATM to enforce required default also for bool
				return SwapRes::notFound;
			} else if constexpr (std::is_integral<Type>::value) {
				if (value == JSONReaderConst::setMeInt) {
					return SwapRes::errorMissing;
				}
			} else if constexpr (std::is_floating_point<Type>::value) {
				if (qIsNaN(value)) {
					return SwapRes::errorMissing;
				}
			} else if constexpr (std::is_same<Type, QStringList>::value) {
				if (!value.isEmpty() && value[0] == JSONReaderConst::setMe) {
					return SwapRes::errorMissing;
				}
			} else if constexpr (std::is_same<Type, std::string>::value) {
				if (!value.empty() && value == JSONReaderConst::setMeSS) {
					return SwapRes::errorMissing;
				}
			} else if constexpr (std::is_enum<Type>::value) {
				if (value == Type::null) {
					return SwapRes::errorMissing;
				}
			} else {
				//poor man static assert that will also print for which type it failed
				typedef typename Type::something_made_up X;

				X x; //To avoid complain that X is defined but not used
			}
			return SwapRes::notFound;
		}

		if constexpr (std::is_same_v<Type, QByteArray>) {
			value.clear();
			value.append(obj->GetString());
			return SwapRes::swapped;
		} else if constexpr (std::is_same_v<Type, QString>) {
			value.clear();
			value.append(obj->GetString());
			return SwapRes::swapped;
		} else if constexpr (std::is_same_v<Type, std::string>) {
			value.clear();
			value.append(obj->GetString());
			return SwapRes::swapped;
		} else if constexpr (std::is_same_v<Type, QStringList>) {
			value.clear();
			for (auto& iter : obj->GetArray()) {
				value.append(iter.GetString());
			}
			return SwapRes::swapped;
			// not ok in this way because we do not check the type
			//		} else if constexpr (std::is_same<Type, double>::value) {
			//			value = obj->GetDouble();
			//			return SwapRes::swapped;
		} else if constexpr (std::is_enum_v<Type>) {
			magic_enum::fromString(obj->GetString(), value);
			return SwapRes::swapped;
		} else { //This should handle all the other
			if (obj->GetType() == rapidjson::Type::kNumberType && std::is_arithmetic_v<Type>) {
				//as suggested in https://github.com/Tencent/rapidjson/issues/823
				value = obj->GetDouble();
				return SwapRes::swapped;
			} else if (obj->template Is<Type>()) {
				value = obj->template Get<Type>();
				return SwapRes::swapped;
			}
			mismatchedType = obj->GetType();
			return SwapRes::typeMismatch;
		}
	}

      public:
	bool parse(const std::string& raw);
	bool parse(const QByteArray& raw);
	bool parse(const char* raw);

	template <typename Type>
	bool getta(const char* path, Type& def) {
		const auto& ptr = rapidjson::Pointer(path);
		if (!ptr.IsValid()) {
			throw ExceptionV2(QSL("invalid json path: ") + path + QSL(" Error: ") + asString(ptr.GetParseErrorCode()));
		}
		rapidjson::Value* val = ptr.Get(json);
		if (val) {
			swapper(val, def);
			rapidjson::Pointer(path).Erase(json);
			return true;
		}
		return false;
	}

	template <typename Type>
	Type rq(const char* path) {
		Type type;
		getta(path, type);
		return type;
	}

	static QByteArray subJsonRender(rapidjson::Value* el, bool pretty = true);

	QByteArray jsonRender();

	//FIXME fare overload perché funzioni anche con tipi primitivi
	bool stageClear(rapidjson::Value* el, bool verbose = true);

	bool stageClear();
};
