#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_CONCEPT_ISRVALUE_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_CONCEPT_ISRVALUE_H

#include <concepts>
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
concept isEnum = std::is_enum_v<T>;


#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_CONCEPT_ISRVALUE_H
