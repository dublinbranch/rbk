#ifndef JEMUTIL_H
#define JEMUTIL_H

#include <cstdint>
#include <source_location>
#include <string>

namespace JEMUtil {
void        refreshStatsCache();
uint64_t    readU64(const char* name);
std::string memStatsHeader();

std::string memStatsRow(std::string_view stage, std::string_view row);
std::string memStatsRow(std::string_view stage, const std::source_location& loc = std::source_location::current());

/// Full malloc_stats_print text (bins, arenas, mutexes). What /jemalloc/stats returns.
std::string statsPrint();
/// Embedded ClickHouse-derived UI for GET /jemalloc.
std::string pageHtml();

} // namespace JEMUtil

#endif // JEMUTIL_H
