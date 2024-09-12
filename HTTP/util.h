#pragma once

#include <string>

struct HRef {
	enum Target {
		_blank,
		_self,
		_parent,
	} target = Target::_blank;

	std::string url;
	std::string text;
	std::string _class;

	//this is what we will use 99% of the time
	static std::string compose(std::string_view url, std::string_view text);
	std::string compose();
};
