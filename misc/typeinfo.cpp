#include "typeinfo.h"
#include <cxxabi.h>

std::string demangle(const char* name) {
	int status = 0;

	auto res = abi::__cxa_demangle(name, NULL, NULL, &status);

	if (status == 0) {
		std::string cry(res);
		free(res);
		return cry;
	} else {
		//something did not worked, sorry
		return name;
	}
}
