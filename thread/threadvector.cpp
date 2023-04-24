#include "threadvector.h"

void ThreadVector::wait() {
	// wait all thread are finished
	for (auto& thread : *this) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}
