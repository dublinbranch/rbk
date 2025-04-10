#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_STRING_CONCEPT_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_STRING_CONCEPT_H

#include <concepts>
#include <string>
#include <string_view>

// Define a concept that constrains a type to be either std::string or std::string_view so they can be used without a copy
template <typename T>
concept StdStringLike = std::same_as<T, std::string> || std::same_as<T, std::string_view>;

// Define a concept that constrains a type to be either StdStringLike or const char* so they can be eventually with a copy
template <typename T>
concept StdStringable = StdStringLike<T> || std::same_as<T, const char*>;

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_STRING_CONCEPT_H
