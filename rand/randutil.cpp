#include "randutil.h"
#include <cstdlib>

uint rand(uint from, uint to) {
	if (from < to) {
		return (uint)rand() % ((to - from) + 1) + from;
	}
	return 0;
}
