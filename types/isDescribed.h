/*
constexpr bool isMyClassDescribed = boost::describe::has_describe_members<MyClass>::value;
*/

template <typename T>
concept isDescribedClass = boost::describe::has_describe_members<T>::value;
