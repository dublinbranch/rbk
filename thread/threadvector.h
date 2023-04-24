#ifndef THREADVECTOR_H
#define THREADVECTOR_H

#include <thread>
#include <vector>

class ThreadVector : public std::vector<std::thread> {
      public:
	void wait();
};

#endif // THREADVECTOR_H
