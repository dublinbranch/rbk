#pragma once

#include <filesystem>

// Define a user-defined literal for std::filesystem::path
std::filesystem::path operator""_p(const char* path, std::size_t);
