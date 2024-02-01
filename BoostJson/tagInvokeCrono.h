#ifndef HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_TAGINVOKECRONO_H
#define HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_TAGINVOKECRONO_H

#include <boost/json/value.hpp>
#include <chrono>
#include <fmt/printf.h>

namespace bj = boost::json;

bool checkAreSameAmount(const auto& source, const auto& dest) {
	if (source != dest) {
		fmt::print("Precision loss while converting {}, it will result in {}\n\n", source, dest);
		fflush(stdout);
		return false;
	}
	return true;
}

namespace boost {
namespace json {

template <typename Rep, typename Period>
std::chrono::duration<Rep, Period> tag_invoke(bj::value_to_tag<std::chrono::duration<Rep, Period>>, bj::value const& jv) {

	typedef std::chrono::duration<Rep, Period> Dest;

	const auto& obj = jv.as_object();
	if (obj.size() != 1) {
		//throw std::system_error(std::string(R"(invalid time period obj, it must be like {"seconds":123}, you can use minutes and hours)"));
	}
	auto key = obj.begin()->key();
	auto val = obj.begin()->value().to_number<Rep>();
	if (key == "seconds") {
		std::chrono::seconds source(val);
		auto                 conv = std::chrono::duration_cast<Dest>(source);
		checkAreSameAmount(source, conv);
		return conv;
	}
	if (key == "minutes") {
		std::chrono::minutes source(val);
		auto                 conv = std::chrono::duration_cast<Dest>(source);
		checkAreSameAmount(source, conv);
		return conv;
	}
	if (key == "hours") {
		std::chrono::hours source(val);
		auto               conv = std::chrono::duration_cast<Dest>(source);
		checkAreSameAmount(source, conv);
		return conv;
	}
	fmt::print(R"({} is an unsupported time amount for time period obj, use seconds minutes or hours\n)", std::string_view(key));
	fflush(stdout);
	abort();
}

template <typename Rep, typename Period>
void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, const std::chrono::duration<Rep, Period>& t) {
	using namespace std::string_literals;
	using namespace std::chrono_literals;

	if constexpr (std::is_same_v<Period, std::ratio<3600>>) {
		jv = {{"hours", static_cast<Rep>(t / 1.0h)}};
	} else if constexpr (std::is_same_v<Period, std::ratio<60>>) {
		jv = {{"minutes", static_cast<Rep>(t / 1.0min)}};
	} else if constexpr (std::is_same_v<Period, std::ratio<1>>) {
		jv = {{"seconds", static_cast<Rep>(t / 1.0s)}};
	} else {
		// poor man static assert that will also print for which type it failed
		using X = typename Period::something_made_up;

		X y;     // To avoid complain that X is defined but not used
		(void)y; // TO avoid complain that y is unused
	}
}

} // namespace json
} // namespace boost
#endif // HOME_ROY_PUBLIC_GOOGLEADSLISTENER_RBK_BOOSTJSON_TAGINVOKECRONO_H
