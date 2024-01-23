#ifndef INTRUSIVEDEBUG_H
#define INTRUSIVEDEBUG_H

#include "rbk/misc/typeinfo.h"
#include <fmt/format.h>
#include <string>
#include <vector>
#include <boost/version.hpp>


struct PushMe {
	PushMe();
	static void                            push(const char* str);
	static void                            pop();
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

#if BOOST_VERSION == 108300
#include "rbk/BoostJson/override/value_to_108300.hpp"
#elif BOOST_VERSION == 108400
#include "rbk/BoostJson/override/value_to_108400.hpp"
#else
#error "Unsupported Boost version for the intrusive HTTP Json customize one from json/detail/value_to.hpp"
#endif

#endif // INTRUSIVEDEBUG_H
