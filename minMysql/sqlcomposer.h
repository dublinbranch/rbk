#pragma once

#include "rbk/fmtExtra/includeMe.h"

class SScol {
      public:
	class Value {
	      public:
		Value() = default;
		Value(const std::string& s, bool noQuote_ = false, bool noEscape_ = false);

		std::string val;
		bool        noQuote  = false;
		bool        noEscape = false;
	};

	SScol() = default;

	SScol(const std::string_view& key_, const Value& val_) {
		key = key_;
		val = val_;
	}

	template <typename V>
	SScol(const std::string_view& key_, const V& val_) {
		key = key_;
		setVal(val_);
	}

	template <typename K, typename V>
	SScol(const K& key_, const V& val_) {
		setKey(key_);
		setVal(val_);
	}

	template <typename T>
	void setKey(const T& key_) {
		key = F("{}", key_);
	}

	template <typename T>
	void setVal(const T& value) {
		if constexpr (std::is_arithmetic<T>::value) {
			aritmetic = true;
		}
		val.val = F("{}", value);
	}

	bool        aritmetic = false;
	std::string key;
	Value       val;

      private:
};

class DB;
//is a vector to keep the order of the pushed stuff intact
class SqlComposer : public std::vector<SScol> {
      public:
	SqlComposer(DB* db_, const std::string& separator_ = ",");

	void push(const SScol& col, bool force = false);

	template <typename K, typename V>
	void push(const K& key_, const V& val_, bool force = false) {
		push({key_, val_}, force);
	}

	std::string compose() const;
	QString     composeQS() const;
	bool        valid     = true;
	std::string separator = ",";
	//change into " AS " for INSERT INTO / SELECT
	std::string joiner    = " = ";
	bool        isASelect = false;

	void setIsASelect();

      private:
	size_type longestKey = 0;
	size_type longestVal = 0;
	DB*       db         = nullptr;
};
