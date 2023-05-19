#pragma once

#include "rbk/fmtExtra/includeMe.h"
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
		key = F("`{}`", key_);
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

      private:
};

class DB;
//is a vector to keep the order of the pushed stuff intact
class SqlComposer : public std::vector<SScol> {
      private:
	struct PrivateTag {};

	size_type   longestKey = 0;
	size_type   longestVal = 0;
	DB*         db         = nullptr;
	std::string table;

      public:
	SqlComposer(PrivateTag){};
	explicit SqlComposer(DB* db_, const std::string& separator_ = ",");

	void push(const SScol& col, bool force = false);

	template <typename K, typename V>
	void push(const K& key_, const V& val_, bool force = false) {
		push(SScol{key_, val_}, force);
	}

	template <class... T>
	void setTable(T&&... strings) {
		//TODO https://www.linkedin.com/pulse/nth-element-variadic-pack-extraction-alex-dathskovsky-/?trk=pulse-article_more-articles_related-content-card
		constexpr auto size = sizeof...(strings);
		//based on number of parameter I know if this is a DB + TABLE or a single entity, so I join or not them
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

	[[nodiscard]] std::string compose() const;
	[[nodiscard]] QString     composeQS() const;
	[[nodiscard]] std::string composeSelect();
	[[nodiscard]] std::string composeUpdate() const;
	[[nodiscard]] std::string composeInsert() const;
	[[nodiscard]] std::string composeDelete() const;

	bool        valid     = true;
	std::string separator = ",";
	//change into " AS " for INSERT INTO / SELECT
	std::string joiner    = " = ";
	bool        isASelect = false;

	std::unique_ptr<SqlComposer> where;
	void                         setIsASelect();
	std::string                  getTable() const;
};
