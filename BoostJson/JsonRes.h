#ifndef JSONRES_H
#define JSONRES_H

#include <boost/json.hpp>
#include <string>

struct JsonRes {
	std::string        raw;
	boost::json::value json;
	//How many bytes the parser consumed before it stopped. On an error this is WHERE
	//it failed, and that is 0 when the very first byte is already invalid ("xyz", "}"),
	//so position is NOT an error flag. Ask failed() / ec instead.
	size_t                    position = 0;
	boost::system::error_code ec;
	boost::json::storage_ptr  storage;
	//The one error check. True for a syntax error, a truncated text, trailing garbage
	//or an empty input.
	[[nodiscard]] bool        failed() const { return ec.failed(); }
	[[nodiscard]] std::string composeErrorMsg() const;
};

#endif // JSONRES_H
