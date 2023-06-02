#ifndef CHECKOPTIONALARESET_H
#define CHECKOPTIONALARESET_H

#include <boost/describe.hpp>
#include <boost/describe/class.hpp>
#include <fmt/core.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

template <typename T, typename Enable = void>
struct isOptional : std::false_type {};

template <typename T>
struct isOptional<std::optional<T>> : std::true_type {};

template <class T>
bool hasValue(const T& t) {
	if constexpr (isOptional<T>()) {
		if (!t.has_value()) {
			return false;
		}
	}
	return true;
}

template <class T>
struct isVector : std::false_type {};

template <class T>
struct isVector<std::vector<T>> : std::true_type {};

template <class T>
concept canBeChecked = requires(const T& t) {
	checkOptionalAreSet(t);
};

template <class T,
          class Bd = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
          class Md = boost::describe::describe_members<T, boost::describe::mod_any_access>,
          class En = std::enable_if_t<!std::is_union<T>::value>>
bool checkOptionalAreSet(T const& t, std::string basePath="") {
	boost::mp11::mp_for_each<Bd>([&](auto D) {
		using B = typename decltype(D)::type;
		checkOptionalAreSet((B const&)t, basePath);
	});

	boost::mp11::mp_for_each<Md>([&](auto D) {
		std::string name = D.name;
		std::string path = fmt::format("{}/{}", basePath, name);
		//x is the base object, *D.pointer is the pointer to the internal element, the dynamic way of writing x->element
		//auto& el = t.*D.pointer;
		//and this gives the type
		decltype(t.*D.pointer) dd{};
		using Type = std::remove_cvref_t<decltype(t.*D.pointer)>;
		Type dd2;

		if constexpr (isVector<Type>()) {
			int i = 0;
			for (auto& row : t.*D.pointer) {
				path = fmt::format("{}/{}", path, i);
				checkOptionalAreSet(row, path);
				i++;
			}
		} else if constexpr (canBeChecked<decltype(t.*D.pointer)>) {
			checkOptionalAreSet(t.*D.pointer, path);
		} else {
			if (!hasValue(t.*D.pointer)) {
				throw std::runtime_error(path + " is missing initialization value");
			}
		}
	});

	return true;
}

#endif // CHECKOPTIONALARESET_H
