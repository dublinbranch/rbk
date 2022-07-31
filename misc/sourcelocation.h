#ifndef SOURCELOCATION_H
#define SOURCELOCATION_H

#include <string>
#include <source_location>

std::string locationFull(const std::source_location location= 
		std::source_location::current());

#endif // SOURCELOCATION_H
