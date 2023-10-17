#ifndef ISOPTIONAL_H
#define ISOPTIONAL_H
//from https://stackoverflow.com/questions/62757803/check-if-some-type-is-instantion-of-template-class-stdoptional
#include <optional>
#include <type_traits>

template<typename>   constexpr bool is_optional_impl = false;
template<typename T> constexpr bool is_optional_impl<std::optional<T>> = true;

template<typename T>
constexpr bool is_optional = is_optional_impl<std::remove_cvref_t<T>>;

#endif // ISOPTIONAL_H
