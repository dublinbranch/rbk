#pragma once

#include <string>

struct HRef {
	enum Target : int {
		_blank = 0,
		_self = 1,
		_parent = 2,
	} target = Target::_blank;

	std::string url;
	std::string text;
	std::string _class;

	//this is what we will use 99% of the time
	static std::string compose(std::string_view url, std::string_view text);
	std::string compose();
};
