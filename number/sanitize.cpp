#include "sanitize.h"
#include <cmath>

double clamp(double val, const Clamp& opt) {
	if (std::isnan(val)) {
		return opt.nanAs;
	}
	if (std::isinf(val)) {
		return opt.infiniteAs;
	}
	return val;
}

double deNaN(double val) {
	return clamp(val, {0, 0});
}
