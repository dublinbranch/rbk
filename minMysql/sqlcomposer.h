#pragma once

#include "rbk/fmtExtra/includeMe.h"

class SScol {
      public:
	SScol() = default;

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
		val = F("{}", value);
	}

	bool        aritmetic = false;
	std::string key;
	std::string val;

      private:
};

class DB;
//is a vector to keep the order of the pushed stuff intact
class SqlComposer : public std::vector<SScol> {
      public:
	SqlComposer(DB* db_, bool forInsert_ = false);
	void push(const SScol& col);

	std::string compose() const;
	bool        valid     = true;
	bool        forInsert = false;

      private:
	size_type longestKey = 0;
	size_type longestVal = 0;
	DB*       db         = nullptr;
};
