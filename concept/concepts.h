#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_CONCEPT_ISRVALUE_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_CONCEPT_ISRVALUE_H

#include <concepts>
#include <string>
class QByteArray;

//This do not work!! to constrain a template! no idea why, do not use
//template <typename T>
//concept isRValue = std::is_rvalue_reference_v<T>;
namespace RBK {
template <typename T>
struct swapTypeToTag {};

} // namespace RBK

template <typename T>
concept TagInvokable = requires(const RBK::swapTypeToTag<T> tag, const QByteArray& source) {
	{ tag_invoke(tag, source) };
};

template <typename T>
concept TagInvokable_SS = requires(const RBK::swapTypeToTag<T> tag, const std::string& source) {
	{ tag_invoke(tag, source) };
};

template <typename T>
concept isEnum = std::is_enum_v<T>;

template <class T>
concept arithmetic = std::integral<T> || std::floating_point<T>;

template<typename T>
concept IntegerNotBool =
    std::is_integral_v<T> &&
    !std::is_same_v<std::remove_cv_t<T>, bool>;

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_CONCEPT_ISRVALUE_H
