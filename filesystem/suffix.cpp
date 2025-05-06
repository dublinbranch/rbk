#include "suffix.h"

std::filesystem::path operator""_p(const char* path, std::size_t) {
	return std::filesystem::path(path);
}
