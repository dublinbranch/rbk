#include "util.h"
#include "rbk/fmtExtra/dynamic.h"
#include "rbk/magicEnum/magic_from_string.hpp"

using namespace std;

std::string HRef::compose(std::string_view url, std::string_view text) {
	HRef a;
	a.url  = url;
	a.text = text;
	return a.compose();
}

std::string HRef::compose() {
	string c;
	if (!_class.empty()) {
		c = F(" class='{}'", _class);
	}
	return F(R"(<a {} target="{}" href='{}'>{}</a>)", c, asSWString(target), url, text);
}
