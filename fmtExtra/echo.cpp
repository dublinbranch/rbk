#include "echo.h"
#include "fmt/format.h"
void echo(const std::string_view s) {
	fmt::print("{}\n", s);
}

void echo(const std::string& s) {
	fmt::print("{}\n", s);
}
