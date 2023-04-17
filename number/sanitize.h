#ifndef SANITIZE_H
#define SANITIZE_H

#include <limits>
struct Clamp {
	double infiniteAs = std::numeric_limits<double>::max();
	double nanAs      = std::numeric_limits<double>::quiet_NaN();
};

double clamp(double val, const Clamp& opt);

double deNaN(double val);

#endif // SANITIZE_H
