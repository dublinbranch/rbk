#include "doubleoperator.h"
#include <cmath>

namespace RBK {
bool compare(double lhs, double rhs, double ths) {
	return fabs(lhs - rhs) < ths;
}

bool less(double lhs, double rhs, double ths) {
	return rhs - lhs > ths;
}

bool Less::operator()(const double &a, const double &b) const {
	return RBK::less(a, b);
}

} // namespace RBK
