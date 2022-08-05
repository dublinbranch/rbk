#include "sourcelocation.h"
#include <fmt/format.h>
std::string locationFull(const sourceLocation location) {
	return fmt::format("{}:{} in {}", location.file_name(), location.line(), location.function_name());
}
