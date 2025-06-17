#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_INTRUSIVEDEBUG_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_INTRUSIVEDEBUG_H

#include <boost/preprocessor/stringize.hpp>

//IF THIS file is included. well we want to use it... and to work we need to override the default one!
//so if the default one is already included we must block compilation and report the error
#ifdef BOOST_JSON_DETAIL_VALUE_TO_HPP
#error "You must include this file before the default one (just put in the first line the #include "rbk/BoostJson/intrusivedebug.h")"
#endif

#include "rbk/misc/typeinfo.h"
#include <boost/json/fwd.hpp>
#include <boost/system/error_code.hpp>
#include <boost/version.hpp>
#include <fmt/format.h>
#include <string>
#include <vector>

struct BJIntrusive {
	BJIntrusive();
	static void                             push(const char* str);
	static void                             pop();
	static std::string                      composePath();
	static std::string                      composeMessage(boost::json::value* original_, boost::json::value target);
	static inline std::string               message;
	static inline std::vector<const char*>  path;
	static inline std::string_view          key;
	static inline boost::system::error_code error;
	static inline boost::json::value*       original = nullptr;
};

#ifndef BOOST_PATH_PUSH
#define BOOST_PATH_PUSH(x) (BJIntrusive::push(x));
#endif

#ifndef BOOST_PATH_POP
#define BOOST_PATH_POP (BJIntrusive::pop());
#endif

#ifndef BOOST_MESSAGE
#define BOOST_MESSAGE(x) (BJIntrusive::message = (x));
#endif

#define BOOST_JSON_INTRUSIVE

//#if BOOST_VERSION == 108300
//#include "rbk/BoostJson/override/value_to_108300.hpp"
#if BOOST_VERSION == 108400
#include "rbk/BoostJson/override/value_to_108400.hpp"
#elif BOOST_VERSION == 108500
#include "rbk/BoostJson/override/value_to_108500.hpp"
#elif BOOST_VERSION == 108600
#include "rbk/BoostJson/override/value_to_108600.hpp"
#elif BOOST_VERSION == 108700
#include "rbk/BoostJson/override/value_to_108700.hpp"
#elif BOOST_VERSION == 108800
#include "rbk/BoostJson/override/value_to_108800.hpp"
#else
#pragma message "Unsupported Boost version " BOOST_PP_STRINGIZE(BOOST_VERSION) " for the intrusive HTTP Json customize one from json/detail/value_to.hpp"
#include <boost/json/detail/value_to.hpp>
#endif
#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_INTRUSIVEDEBUG_H
