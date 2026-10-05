#include "runnableV2.h"
#include <chrono>

namespace rbk {

RunnableV2::Result RunnableV2::runnable(std::string_view key, i64 coolDownSec) {
	using namespace std::chrono;
	const i64 now = fakeNowSec > 0 ? fakeNowSec : duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

	std::lock_guard lock(mu);
	auto            it = entries.find(key);
	if (it == entries.end()) {
		entries.emplace(std::string(key), Entry{now, 0, 0});
		return {true, 0, 0};
	}

	auto& e = it->second;
	if (coolDownSec <= 0 || now < e.lastRunSec || now - e.lastRunSec >= coolDownSec) {
		Result r{true, e.suppressed, e.firstSuppressedSec};
		e = Entry{now, 0, 0};
		return r;
	}

	if (e.suppressed == 0) {
		e.firstSuppressedSec = now;
	}
	++e.suppressed;
	return {};
}

RunnableV2::Result RunnableV2::operator()(std::string_view key, i64 coolDownSec) {
	return runnable(key, coolDownSec);
}

} // namespace rbk
