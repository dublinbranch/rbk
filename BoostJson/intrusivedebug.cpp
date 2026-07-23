#include "intrusivedebug.h"
#include "extra.h"
#include "rbk/BoostJson/math.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/fmtExtra/includeMe.h"
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace bj = boost::json;

std::string BJIntrusive::composePath() {
	if (path.empty()) {
		//This is a json pointer, is supposed to be empty in this case
		return {};
	}
	return fmt::format("/{}", fmt::join(path, "/"));
}

std::string BJIntrusive::pathLabel() {
	return path.empty() ? std::string("(root)") : composePath();
}

namespace {

bj::value* valueAtPointer(bj::value& root, std::string_view ptr, boost::system::error_code& ec) {
	if (ptr.empty()) {
		return &root;
	}
	return root.find_pointer(ptr, ec);
}

} // namespace

//TODO add a struct for options, one of those is if to add the original JSON in the error message
std::string BJIntrusive::composeMessage(bj::value* original_, bj::value target) {
	original     = original_;
	auto pathStr = composePath();
	if (error == bj::error::size_mismatch) {
		boost::system::error_code ec;
		auto                      ptrO = valueAtPointer(*original_, pathStr, ec);
		auto                      ptrT = valueAtPointer(target, pathStr, ec);

		if (ptrO && ptrT && ptrO->is_object() && ptrT->is_object()) {
			auto extras = extraJsonKeyPaths(ptrO->as_object(), ptrT->as_object(), pathStr);
			if (extras.size() == 1) {
				return fmt::format("Extra JSON key: {}", extras.front());
			}
			if (!extras.empty()) {
				return fmt::format("Extra JSON keys:\n{}", fmt::join(extras, "\n"));
			}
			return fmt::format("Extra JSON key(s) in {} ({})\n{}", pathLabel(), message,
			                   pretty_print(subtractJson(ptrO->as_object(), ptrT->as_object(), pathStr)));
		}
		throw ExceptionV2(F("Impossible to find the JSON path {}", pathLabel()));
	}
	return message;
}

BJIntrusive::BJIntrusive() {
	BJIntrusive::path.reserve(32);
}

void BJIntrusive::push(const char* str) {
	//fmt::println("{}", composePath());
	BJIntrusive::path.push_back(str);
}

void BJIntrusive::pop() {
	BJIntrusive::path.pop_back();
}
