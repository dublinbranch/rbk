#include "intrusivedebug.h"
#include "extra.h"
#include "rbk/BoostJson/math.h"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/fmtExtra/includeMe.h"
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace bj = boost::json;

std::string BJIntrusive::composePath() {
	return fmt::format("/{}", fmt::join(path, "/"));
}

//TODO add a struct for options, one of those is if to add the original JSON in the error message
std::string BJIntrusive::composeMessage(bj::value* original_, bj::value target) {
	original     = original_;
	auto pathStr = composePath();
	if (error == bj::error::size_mismatch) {
		//Extra element is
		std::error_code ec;
		auto            ptrO = original->find_pointer(pathStr, ec);
		auto            ptrT = target.find_pointer(pathStr, ec);

		if (ptrO && ptrT) {
			auto res = pretty_print(subtractJson(ptrO->as_object(), ptrT->as_object(), pathStr));
			return fmt::format("Found extra element in path {}\n{}", composePath(), res);
		} else {
			throw ExceptionV2(F("Impossible to find the JSON path {}", pathStr));
		}
		return fmt::format("{}: {}", composePath(), message);
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
