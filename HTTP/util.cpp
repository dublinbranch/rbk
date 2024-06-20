#include "util.h"
#include "rbk/fmtExtra/dynamic.h"
#include "rbk/magicEnum/magic_from_string.hpp"

std::string HRef::compose(std::string_view url, std::string_view text) {
	HRef a;
	a.url  = url;
	a.text = text;
	return a.compose();
}

std::string HRef::compose() {
	return F(R"(<a target="{}" href='{}'>{}</a>)", asSWString(target), url, text);
}
