#ifndef SOURCELOCATION_H
#define SOURCELOCATION_H

#if defined(__clang__)
#include <experimental/source_location>
using sourceLocation = std::experimental::source_location;
#else
#include <source_location>
using sourceLocation = std::source_location;
#endif
#include <string>

std::string locationFull(const sourceLocation location =
                             sourceLocation::current());

#endif // SOURCELOCATION_H
