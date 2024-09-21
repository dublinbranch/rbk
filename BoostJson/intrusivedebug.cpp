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

std::string BJIntrusive::composeMessage(bj::value* original_, bj::value target) {
	original  = original_;
	auto path = composePath();
	if (error == bj::error::size_mismatch) {
		//Extra element is
		std::error_code ec;
		auto            ptrO = original->find_pointer(path, ec);
		auto            ptrT = target.find_pointer(path, ec);

		if (ptrO && ptrT) {
			auto res = pretty_print(subtractJson(ptrO->as_object(), ptrT->as_object(), path));
			return fmt::format("Found extra element in path {}\n{}", composePath(), res);
		} else {
			throw ExceptionV2(F("Impossible to find the JSON path {}", path));
		}
		return fmt::format("{}: {}", composePath(), message);
	}
	return message;
}

BJIntrusive::BJIntrusive() {
	BJIntrusive::path.reserve(32);
}

void BJIntrusive::push(const char* str) {
	BJIntrusive::path.push_back(str);
}

void BJIntrusive::pop() {
	BJIntrusive::path.pop_back();
}
