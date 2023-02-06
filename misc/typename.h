#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MISC_TYPENAME_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MISC_TYPENAME_H
#include <cstdlib>
#include <cxxabi.h>
#include <string>

template <typename T>
std::string typeName() {
	int         status;
	std::string tname          = typeid(T).name();
	char*       demangled_name = abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status);
	if (status == 0) {
		tname = demangled_name;
		std::free(demangled_name);
	}
	return tname;
}

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_MISC_TYPENAME_H
