#pragma once

#include "rbk/fmtExtra/includeMe.h"
#include "rbk/string/util.h"
#include <memory>

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
      private:
	struct PrivateTag {};

	size_type longestKey = 0;
	size_type longestVal = 0;
	DB*       db         = nullptr;

      public:
	SqlComposer(PrivateTag){};
	explicit SqlComposer(DB* db_, const std::string& separator_ = ",");

	void push(const SScol& col, bool force = false);

	template <class... T>
	void setTable(T&&... strings) {
		//TODO https://www.linkedin.com/pulse/nth-element-variadic-pack-extraction-alex-dathskovsky-/?trk=pulse-article_more-articles_related-content-card
		constexpr auto   size = sizeof...(strings);
		std::tuple<T...> tuple(strings...);
		auto             first = std::get<0>(tuple);

		switch (size) {
		case 1:
			table = toStdString(first);
			break;
		case 2: {
			auto x1 = toStdString(first);
			if (x1.ends_with(".")) {
				table = F("{}{}", strings...);
			} else {
				table = F("{}.{}", strings...);
			}
			break;
		}
		default:
			static_assert(size < 3, "invalid number of parameter, 2 or 1");
			break;
		}
	}

	void setTable(const std::string& table_) {
		table = table_;
	}

	template <typename K, typename V>
	void push(const K& key_, const V& val_, bool force = false) {
		push({key_, val_}, force);
	}

	[[nodiscard]] std::string compose() const;
	[[nodiscard]] QString     composeQS() const;

	[[nodiscard]] std::string composeUpdate() const;
	bool                      valid     = true;
	std::string               separator = ",";
	//change into " AS " for INSERT INTO / SELECT
	std::string joiner    = " = ";
	bool        isASelect = false;
	std::string table;

	std::unique_ptr<SqlComposer> where;
	void                         setIsASelect();
};
