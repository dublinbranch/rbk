#ifndef TYPEINFO_H
#define TYPEINFO_H

#include <string>
#include <typeinfo>

std::string demangle(const char* name);

template <typename Type>
std::string getTypeName() {
	auto name = typeid(Type).name();
	return demangle(name);
}

#endif // TYPEINFO_H
