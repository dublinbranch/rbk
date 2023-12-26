#include "util.h"

std::optional<boost::json::value> getValueFromNonsense(const boost::json::array& nonsense, std::string_view keyName, std::string_view keyValue, std::string_view valueName) {
	for (auto& element : nonsense) {
		auto el = element.as_object();
		if (el[keyName].as_string() == keyValue) {
			return el[valueName];
		}
	}
	return {};
}
