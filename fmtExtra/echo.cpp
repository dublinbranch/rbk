#include "echo.h"
#include "fmt/format.h"
void echo(const std::string_view s) {
	fmt::print("{}", s);
}

void echo(const std::string& s) {
	fmt::print("{}", s);
}
