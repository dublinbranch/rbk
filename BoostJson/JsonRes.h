#ifndef JSONRES_H
#define JSONRES_H

#include <boost/json.hpp>
#include <string>

struct JsonRes {
	std::string        raw;
	boost::json::value json;
	//if position is set, it means there was an error and this is the position
	size_t                    position = 0;
	boost::system::error_code ec;
	boost::json::storage_ptr  storage;
	[[nodiscard]] std::string composeErrorMsg() const;
};

#endif // JSONRES_H
