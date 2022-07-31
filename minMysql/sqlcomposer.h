#pragma once

#include "min_mysql.h"
#include <QList>
#include <QString>

class SScol {
      public:
	SScol() = default;
	template <typename K, typename V>
	SScol(const K& key, const V& val) {
		setKey(key);
		setVal(val);
	}
	QString getKey() const;
	template <typename T>
	void setKey(const T& value) {
		key = QSL("%1").arg(value);
	}

	template <typename T>
	void setVal(const T& value) {
		aritmetic = std::is_arithmetic<T>::value;
		if constexpr (std::is_same<QByteArray, T>::value){
			val       = value;
		}else{
			val       = QSL("%1").arg(value);
		}
		
	}

	QString getVal() const;

	QString assemble(int padding = 1) const;

      private:
	bool    aritmetic = false;
	QString key;
	QString val;
};

class SqlComposer {
      public:
	void push(const SScol& col);
	QString compose() const;

	bool valid = true;
      private:
	std::vector<SScol> vector;
	int longestKey = 0;
};
