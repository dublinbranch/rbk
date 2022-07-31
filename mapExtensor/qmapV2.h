#pragma once

#include "rbk/QStacker/exceptionv2.h"
#include "fmt/core.h"
#include "rbk/fmtExtra/customformatter.h"
#include <QDate>
#include <QMap>

class MissingKeyEX : public ExceptionV2 {
      public:
	MissingKeyEX(const QString& _msg)
	    : ExceptionV2(_msg, 6) {
		forcePrint = true;
	}
};

template <class Key, class T>
class QMapV2 : public QMap<Key, T> {
      public:
	/**
	 * use like
	 * QMap x
	 * if( auto v = x.fetch(); v){
	 *	ok present
	 * res = *v.value();
	 * }else{
	 *	missing
	 * }
	 */
	[[nodiscard]] auto fetch(const Key& key) const {
		struct OK {
			operator bool() const {
				return present;
			}
			const T* value   = nullptr;
			bool     present = false;
		};
		auto iter = this->find(key);
		if (iter == this->end()) {
			return OK();
		} else {
			return OK{&iter.value(), true};
		}
	}

	void get(const Key& key, T& val) const {
		getReal(key, val);
	}

	bool get(const Key& key, T& val, const T& def) const {
		return getReal(key, val, &def);
	}

	bool getReal(const Key& key, T& val, const T* def = nullptr) const {
		if (auto v = this->fetch(key); v) {
			val = *v.value;
			return true;
		} else {
			if (def) {
				val = *def;
				return false;
			} else {
				QString stringKey;
				if constexpr (std::is_same<Key, QDate>::value) {
					stringKey = key.toString("yyyy-MM-dd");
				} else {
					stringKey = QString(key);
				}
				throw MissingKeyEX(QString("no key > %1 < and missing default value, what should I do ?").arg(stringKey));
			}
		}
	}

	T get(const Key& key, const T& def) const {
		T v;
		getReal(key, v, &def);
		return v;
	}

	T get(const Key& key) const {
		T v;
		getReal(key, v);
		return v;
	}

	[[nodiscard]] const auto& operator[](const Key& k) const {
		if (auto iter = this->find(k); iter != this->end()) {
			return *iter;
		} else {
			throw ExceptionV2(fmt::format("key {} not found in {}", k, __PRETTY_FUNCTION__));
		}
	}
	/*
	 *For obscure reason the compiler elect to use the const version, ignoring the base class NON const one
	 * so we redefine ...
	 */
	[[nodiscard]] auto& operator[](const Key& k) {
		return QMap<Key, T>::operator[](k);
	}
};
