#include "ieee754.h"
#include <math.h>

double deNaN(double x) {
	if (x != x) { // nan != nan
		return 0;
	}
	if (isinf(x)) {
		return 0; // usually we have inf after division by zero, so is ok to write 0
	}
	return x;
}
