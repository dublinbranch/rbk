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
 :root {
  --bg: #f3f4f6; --card: #fff; --fg: #1f2328; --muted: #59636e; --faint: #8c959f;
  --line: #e1e4e8; --head: #eef1f4; --zebra: #f7f8fa; --hover: #e8f0fb;
  --accent: #0969da; --warn: #b42318; --warn-bg: #fdecea; --shadow: 0 1px 2px rgba(31,35,40,.06);
 }
 @media (prefers-color-scheme: dark) {
  :root {
   --bg: #0d1117; --card: #161b22; --fg: #e6edf3; --muted: #9198a1; --faint: #6e7681;
   --line: #2d333b; --head: #1f252d; --zebra: #1a2028; --hover: #1d2a3b;
   --accent: #58a6ff; --warn: #ff7b72; --warn-bg: #3b1d1d; --shadow: none;
  }
 }
 * { box-sizing: border-box; }
 body { font: 14px/1.45 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif; margin: 0; padding: 0 20px 32px; color: var(--fg); background: var(--bg); }
 a { color: var(--accent); text-decoration: none; }
 a:hover { text-decoration: underline; }
 h1 { font-size: 20px; font-weight: 600; margin: 0; padding: 18px 0 8px; }
 nav { position: sticky; top: 0; z-index: 1; display: flex; flex-wrap: wrap; gap: 6px; padding: 8px 0 12px; margin-bottom: 12px; background: var(--bg); border-bottom: 1px solid var(--line); }
 nav a { padding: 3px 12px; font-size: 13px; color: var(--fg); background: var(--card); border: 1px solid var(--line); border-radius: 999px; }
 nav a:hover { color: var(--accent); border-color: var(--accent); text-decoration: none; }
 /* Every section is a card sized to its table; a section holding sections is only a heading over them */
 section { width: fit-content; max-width: 100%; overflow-x: auto; margin-bottom: 20px; padding: 10px 14px 12px; background: var(--card); border: 1px solid var(--line); border-radius: 8px; box-shadow: var(--shadow); scroll-margin-top: 64px; }
 section:has(section) { width: auto; overflow: visible; padding: 0; background: none; border: 0; box-shadow: none; }
 h2 { font-size: 12px; font-weight: 600; text-transform: uppercase; letter-spacing: .05em; color: var(--muted); margin: 0 0 8px; }
 section:has(section) > h2 { font-size: 16px; text-transform: none; letter-spacing: 0; color: var(--fg); margin: 8px 0 4px; }
 .grid { display: flex; flex-wrap: wrap; gap: 20px; align-items: stretch; margin-bottom: 20px; }
 .grid section { margin-bottom: 0; }
 p { margin: 0 0 8px; }
 table { border-collapse: collapse; }
 th, td { padding: 4px 10px; border-bottom: 1px solid var(--line); text-align: left; vertical-align: baseline; }
 tr:last-child > * { border-bottom: 0; }
 tr.hd th { font-size: 12px; font-weight: 600; color: var(--muted); background: var(--head); white-space: nowrap; }
 tr.hd th.n { text-align: right; }
 tr:not(.hd) th { font-weight: normal; color: var(--muted); white-space: nowrap; }
 tr:nth-child(even of :not(.hd)) > * { background: var(--zebra); }
 tr:not(.hd):hover > * { background: var(--hover); }
 td.n { text-align: right; white-space: nowrap; font: 13px ui-monospace, "DejaVu Sans Mono", Menlo, Consolas, monospace; font-variant-numeric: tabular-nums; }
 .u { display: inline-block; width: 3ch; padding-left: 0.4ch; text-align: left; color: var(--faint); }
 .h { visibility: hidden; }
 .note { color: var(--muted); }
 td.note { font-size: 13px; }
 .warn { color: var(--warn); background: var(--warn-bg); font-weight: 600; padding: 0 6px; border-radius: 4px; }
 pre { margin: 0; padding: 8px 10px; background: var(--zebra); border-radius: 6px; white-space: pre-wrap; font: 12.5px/1.4 ui-monospace, "DejaVu Sans Mono", Menlo, Consolas, monospace; }
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
