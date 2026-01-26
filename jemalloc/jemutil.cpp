#include "jemutil.h"
#include <fmt/core.h>
#include <jemalloc/jemalloc.h>

void JEMUtil::refreshStatsCache() {
	uint64_t epoch = 1;
	size_t   sz    = sizeof(epoch);
	mallctl("epoch", &epoch, &sz, &epoch, sz); // force jemalloc to update cached stats
}

uint64_t JEMUtil::readU64(const char* name) {
	uint64_t v  = 0;
	size_t   sz = sizeof(v);
	if (mallctl(name, &v, &sz, nullptr, 0) != 0)
		return 0;
	return v;
}

std::string JEMUtil::memStatsHeader() {
	return fmt::format("{:>20} {:>9} {:>9} {:>9} {:>9} \t Row",
	                   "Stage", "Allocated", "Active", "Resident", "Mapped");
}

std::string JEMUtil::memStatsRow(std::string_view stage, std::string_view row) {
	refreshStatsCache();
	auto allocated = readU64("stats.allocated"); // bytes currently allocated by app
	auto active    = readU64("stats.active");    // bytes in active pages (includes fragmentation)
	auto resident  = readU64("stats.resident");  // bytes resident for jemalloc
	auto mapped    = readU64("stats.mapped");    // bytes mapped by jemalloc

	return fmt::format("{:>20}{:>9.2f} {:>9.2f} {:>9.2f} {:>9.2f} {}",
	                   stage, allocated / 1048576.0, active / 1048576.0, resident / 1048576.0, mapped / 1048576.0, row);
}

std::string JEMUtil::memStatsRow(std::string_view stage, const std::source_location& loc) {
	auto row = fmt::format("init ({}:{})", loc.file_name(), loc.line());
	return memStatsRow(stage, row);
}
