#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_INTRUSIVEDEBUG_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_INTRUSIVEDEBUG_H

//IF THIS file is included. well we want to use it... and to work we need to override the default one!
//so if the default one is already included we must block compilation and report the error
#ifdef BOOST_JSON_DETAIL_VALUE_TO_HPP
#error "You must include this file before the default one (just put in the first line the #include "rbk/BoostJson/intrusivedebug.h")"
#endif

#include "rbk/misc/typeinfo.h"
#include <boost/version.hpp>
#include <fmt/format.h>
#include <string>
#include <vector>

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

#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_INTRUSIVEDEBUG_H
