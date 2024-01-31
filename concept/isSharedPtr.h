#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_CONCEPT_ISSHAREDPTR_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_CONCEPT_ISSHAREDPTR_H

#include <type_traits>
#include <memory>

template <typename T>
struct is_shared_ptr : std::false_type {};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};


#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_CONCEPT_ISSHAREDPTR_H
