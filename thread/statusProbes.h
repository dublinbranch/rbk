#ifndef HOME_ROY_PUBLIC_DIGITALSPINE_RBK_THREAD_STATUSPROBES_H
#define HOME_ROY_PUBLIC_DIGITALSPINE_RBK_THREAD_STATUSPROBES_H

#include "rbk/number/intTypes.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

/* Readers behind the status page blocks, see statusPageConf.h for the switches.
 * Each one reads /proc, /sys or the kernel right now and returns html: table rows or whole
 * sections, styled by pageHead below. A file the kernel does not have becomes an
 * "unavailable" row, never an error.
 */
namespace statusProbes {

// <!DOCTYPE> up to <body>: the CSS behind every class used here. Shared by the status and the sockets page.
inline constexpr char pageHead[] = R"(<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
 body { font: 14px/1.4 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif; margin: 16px; color: #222; background: #fff; }
 h1 { font-size: 20px; margin: 0 0 4px; }
 h2 { font-size: 15px; margin: 0 0 6px; }
 nav { margin: 0 0 16px; }
 nav a { margin-right: 14px; }
 section { margin-bottom: 22px; }
 .grid { display: flex; flex-wrap: wrap; gap: 24px; align-items: flex-start; margin-bottom: 22px; }
 .grid section { margin-bottom: 0; }
 table { border-collapse: collapse; }
 th, td { padding: 3px 10px; border-bottom: 1px solid #e4e4e4; text-align: left; vertical-align: top; }
 tr.hd th { background: #f2f2f2; border-bottom: 1px solid #c8c8c8; white-space: nowrap; }
 tr:not(.hd) th { font-weight: normal; color: #555; white-space: nowrap; }
 table.t tr:not(.hd):hover td { background: #f8f8f8; }
 td.n { text-align: right; white-space: nowrap; font: 13px ui-monospace, "DejaVu Sans Mono", Menlo, Consolas, monospace; }
 .u { display: inline-block; width: 3ch; padding-left: 0.4ch; text-align: left; color: #777; }
 .h { visibility: hidden; }
 .note { color: #666; }
 .warn { color: #b00020; font-weight: 600; }
 pre { background: #f6f6f6; padding: 8px; white-space: pre-wrap; }
</style>
</head>
<body>
)";

/* Html bits shared by the status pages. Numbers go in right aligned monospace cells (td.n) with a
 * fixed number of decimals per column, and the unit in a fixed width slot after the number, so the
 * decimal points line up down a column.
 */
std::string nInt(u64 v);                                            // 12 345, for columns of integers
std::string nFix(double v, int decimals, std::string_view unit = {}); // the unit slot only when unit is set
std::string nBytes(u64 b);                                          // 2.4 MB, always one decimal (512 B gets a hidden .0)
// Key / value tables mix integers and decimals in one column: these keep everything at one decimal
std::string kInt(u64 v);
std::string kFix(double v, std::string_view unit);
std::string warn(std::string_view msg);
std::string td(std::string_view num); // <td class="n">
// Key / value table row: label | number | note
std::string rowN(std::string_view label, std::string_view num, std::string_view note = {});
// Key / value table row: label | text over the number and note columns
std::string rowT(std::string_view label, std::string_view text);
// <section id><h2>title</h2><table class="kv">rows</table></section>, id may be empty
std::string kvSection(std::string_view id, std::string_view title, std::string_view rows);

// Key / value rows: CPU time, current memory, context switches, I/O, oom score
std::string processRows(double upSec, size_t poolSize);

struct FdCount {
	u64         open = 0;
	std::string kinds; // "socket 12 · pipe 2 ...", empty unless asked for
};
// Every fd we hold, in one fstat() walk. kinds adds the socket / pipe / file / anon split.
FdCount fdCount(bool kinds);

// Host, load and pressure, cgroup and disk sections. leadRows go first in the Host table.
std::string hostSection(const std::vector<std::string>& diskPaths, std::string_view leadRows);
// The Network section. counters = /proc/net, unitNet = systemd IPAccounting of our unit.
// socketsHref links the TCP socket page, empty = no link.
std::string networkSection(bool counters, bool unitNet, std::string_view socketsHref);

struct ThreadKernel {
	bool        ok     = false;
	char        state  = '?';
	int         core   = -1;
	double      cpuSec = 0;
	// Runnable but waiting for a core. -1 = the kernel has no schedstat.
	double      waitSec = -1;
	std::string comm;
};
ThreadKernel threadKernel(int tid);
// <td> state, core, cpu, rq wait
std::string kernelCells(const ThreadKernel& k);

// html table of every thread of the process that is not in poolTids
std::string otherThreadsTable(const std::vector<int>& poolTids);

// html body of the /sockets page
std::string socketsBody(u32 top);

} // namespace statusProbes

#endif // HOME_ROY_PUBLIC_DIGITALSPINE_RBK_THREAD_STATUSPROBES_H
