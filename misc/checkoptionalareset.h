#ifndef CHECKOPTIONALARESET_H
#define CHECKOPTIONALARESET_H

#include <boost/describe.hpp>
#include <boost/describe/class.hpp>
#include <optional>
#include <stdexcept>
#include <string>

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
concept canBeChecked = requires(const T& t) {
	check(t);
};

template <class T,
          class Bd = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
          class Md = boost::describe::describe_members<T, boost::describe::mod_any_access>,
          class En = std::enable_if_t<!std::is_union<T>::value>>
bool checkOptionalAreSet(T const& t) {

	boost::mp11::mp_for_each<Bd>([&](auto D) {
		using B = typename decltype(D)::type;
		checkOptionalAreSet((B const&)t);
	});

	boost::mp11::mp_for_each<Md>([&](auto D) {
		//x is the base object, *D.pointer is the pointer to the internal element, the dynamic way of writing x->element
		//auto& el = t.*D.pointer;
		//and this gives the type
		//decltype(t.*D.pointer) dd{};
		if constexpr (canBeChecked<decltype(t.*D.pointer)>) {
			checkOptionalAreSet(t.*D.pointer);
		} else {
			if (!hasValue(t.*D.pointer)) {
				std::string name = D.name;
				throw std::runtime_error(name + " is missing initialization value");
			}
		}
	});

	return true;
}

#endif // CHECKOPTIONALARESET_H
