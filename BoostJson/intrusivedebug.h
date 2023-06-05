#ifndef INTRUSIVEDEBUG_H
#define INTRUSIVEDEBUG_H

#include "rbk/misc/typeinfo.h"
#include <string>
#include <vector>

struct PushMe {
	PushMe();
	static void push(const char* str);
	static void pop();
	static std::string                     compose();
	static inline std::string              message;
	static inline std::vector<const char*> path;
};

#ifndef BOOST_PATH_PUSH
#define BOOST_PATH_PUSH(x) (PushMe::push(x));
#endif

#ifndef BOOST_PATH_POP
#define BOOST_PATH_POP (PushMe::pop());
#endif

#ifndef BOOST_MESSAGE
#define BOOST_MESSAGE(x) (PushMe::message = (x));
#endif

#include "rbk/BoostJson/override/value_to.h"

#endif // INTRUSIVEDEBUG_H
