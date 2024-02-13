#pragma once

#include "rbk/fmtExtra/includeMe.h"
#include "rbk/misc/intTypes.h"
#include "rbk/number/sanitize.h"
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

	SScol(const Value& key_, const Value& val_)
	    : val(val_) {
		key = key_.val;
	}

	template <typename K>
	SScol(const K& key_, const Value& val_)
	    : val(val_) {
		setKey(key_);
	}

	template <typename K, typename V>
	SScol(const K& key_, const V& val_) {
		setKey(key_);
		setVal(val_);
	}

	template <typename T>
	void setKey(const T& key_) {
		if (verbatim) {
			key = key_;
		} else {
			key = F("`{}`", key_);
		}
	}

	template <typename T>
	void setVal(const T& value) {
		if constexpr (std::is_arithmetic<T>::value) {
			aritmetic  = true;
			auto cheap = value;
			if constexpr (std::is_floating_point<T>::value) {
				cheap = deNaN(value);
			}
			val.val = F("{}", cheap);
		} else {
			val.val = F("{}", value);
		}
	}

	bool        aritmetic = false;
	std::string key;
	Value       val;
	bool        verbatim = false;

      private:
};

class DB;
//is a vector to keep the order of the pushed stuff intact
class SqlComposer : public std::vector<SScol> {
      private:
	struct PrivateTag {};

	u64         longestKey = 0;
	u64         longestVal = 0;
	DB*         db         = nullptr;
	std::string table;

      public:
	explicit SqlComposer(PrivateTag){};
	explicit SqlComposer(DB* db_, const std::string& separator_ = ",");

	void push(const SScol& col, bool replaceIf = false);

	template <typename K, typename V>
	SqlComposer& push(const K& key_, const V& val_, bool replaceIf = false) {
		push(SScol{key_, val_}, replaceIf);
		return *this;
	}

	template <class... T>
	SqlComposer& setTable(T&&... strings) {
		//TODO https://www.linkedin.com/pulse/nth-element-variadic-pack-extraction-alex-dathskovsky-/?trk=pulse-article_more-articles_related-content-card
		constexpr auto size = sizeof...(strings);
		//based on number of parameter I know if this is a DB + TABLE or a single entity, so I join or not them
		std::tuple<T...> tuple(strings...);
		std::string_view first = std::get<0>(tuple);

		switch (size) {
		case 1:
			table = F("{}", first);
			break;
		case 2: {
			if (first.ends_with(".")) {
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
		return *this;
	}

	SqlComposer& setTable(const std::string& table_) {
		if (table_.empty()) {
			throw ExceptionV2("Setting and empty table -.-");
		}
		table = table_;
		return *this;
	}

	[[nodiscard]] std::string compose() const;
	[[nodiscard]] QString     composeQS() const;
	[[nodiscard]] std::string composeSelect();
	[[nodiscard]] std::string composeSelect(const std::string& fields);
	[[nodiscard]] std::string composeSelectAll();
	[[nodiscard]] std::string composeUpdate() const;
	[[nodiscard]] QString     composeUpdateQS() const;
	[[nodiscard]] std::string composeInsert(bool ignore = false) const;
	[[nodiscard]] std::string composeDelete() const;

	[[nodiscard]] std::string composeSelect_V2();

	[[nodiscard]] std::string composeWhere(bool required = false) const;
	[[nodiscard]] std::string composeFrom() const;

	bool        valid     = true;
	std::string separator = ",";
	//change into " AS " for INSERT INTO / SELECT
	std::string joiner = " = ";

	std::unique_ptr<SqlComposer> where = nullptr;

	void        setIsASelect();
	std::string getTable() const;
};
