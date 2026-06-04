#include "wm_config_types.h"

#include "rbk/fmtExtra/dynamic.h"
#include "rbk/string/stringoso.h"

#include <boost/json/conversion.hpp>
#include <boost/json/value.hpp>
#include <stdexcept>

namespace wm_config {

Config defaultConfig() {
	return {};
}

MType mTypeFromString(std::string_view str) {
	if (str == "WM") {
		return MType::WM;
	}
	if (str == "4assi" || str == "_4assi" || str == "4Assi") {
		return MType::_4Assi;
	}
	if (str == "Sewing" || str == "Sew") {
		return MType::Sew;
	}
	throw std::invalid_argument(F("Invalid MType string: {}", str));
}

} // namespace wm_config

namespace boost::json {

wm_config::MType tag_invoke(value_to_tag<wm_config::MType>, const value& jv) {
	if (!jv.is_string()) {
		throw std::invalid_argument("Expected a string for MType");
	}
	return wm_config::mTypeFromString(jv.as_string());
}

void tag_invoke(value_from_tag, value& jv, const wm_config::MType& type) {
	jv = asSWString(type);
}

} // namespace boost::json
