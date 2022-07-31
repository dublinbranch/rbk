#ifndef BETTERENUM_HPP
#define BETTERENUM_HPP

#include "enum.h" //this is https://github.com/aantron/better-enums  https://raw.githubusercontent.com/aantron/better-enums/master/enum.h
#include "fmt/format.h"
#include <vector>

template <class T1, class T2>
void addFlag(T1& t, T2 x) {
	//For some reason -Wall and -pedantic-errors only print a warning here... so we enforce it
	static_assert(std::is_same_v<T1, T2> || std::is_same_v<typename T1::_enumerated, T2>, "The two enum MUST be the same");
	t = static_cast<typename T1::_enumerated>(x | t);
}

template <typename T, typename... Types>
void addFlag(T& var1, Types... var2) {
	for (const auto p : {var2...}) {
		addFlag(var1, p);
	}
}

template <class T>
concept isBetterEnum = requires(const T& t) {
	//just check if some of the better enum function are defined or not
	t._from_index(1);
	t._value;
};

template <typename Type>
requires isBetterEnum<Type>
    std::vector<Type> getEnabledFromBitmask(Type& t) {
	static const auto& values = t._values();
	std::vector<Type>  set;
	for (const auto& c : values) {
		if (t & c) {
			set.push_back(c);
		}
	}
	return set;
}

template <typename Type>
requires isBetterEnum<Type>
    std::vector<std::string> asString(const std::vector<Type>& t, bool verbose = false) {
	std::vector<std::string> res;
	//miserable hack to init the enum, not sure if is ok, but I have no better idea
	Type sum   = Type::_from_index(0);
	sum._value = 0; //And reset immediately
	for (const auto& c : t) {
		if (verbose) {
			auto s = fmt::format("{} ({})", c._to_string(), c._to_integral());
			res.push_back(s);
			addFlag(sum, c);
		} else {
			res.push_back(c._to_string());
		}
	}
	if (verbose) {
		auto s = fmt::format("sum: {}", sum._value);
		res.push_back(s);
	}
	return res;
};

#endif // BETTERENUM_HPP
