#ifndef HOME_ROY_PUBLIC_DIGITALSPINE_RBK_THREAD_STATUSPAGECONF_H
#define HOME_ROY_PUBLIC_DIGITALSPINE_RBK_THREAD_STATUSPAGECONF_H

#include "rbk/number/intTypes.h"
#include <optional>
#include <string>
#include <vector>

/* Blocks of the status page (router.h statusPagePath). Everything is read from /proc, /sys or the
 * kernel when the page loads, nothing is tracked between requests.
 * The page renders on one of the HttpHandler threads, so a slow block stalls every connection of
 * that thread while it runs: switch off whatever turns out to be slow on a box.
 * All optional so config files written before a key keep loading. Absent = on.
 */
struct StatusPageConf {
	// CPU time, current memory, context switches, I/O, oom score
	std::optional<bool> process = true;
	// Kernel side of each thread (CPU, run queue wait, state, core) and the threads outside the pool
	std::optional<bool> threads = true;
	// fstat() of every fd to split them into socket / pipe / anon / file. ~6 ms at 10K sockets.
	std::optional<bool> fdKinds = true;
	// mysql_ping on the main connection. With readTimeout 0 a hung server hangs the page and its thread.
	std::optional<bool> mysqlPing = true;
	// Kernel, load, PSI, clock sync, cgroup limits
	std::optional<bool> host = true;
	// statvfs of each path (logs, spill dir, DB datadir...). Empty = skip.
	std::optional<std::vector<std::string>> diskPaths = std::vector<std::string>{"."};
	// /proc/net and /proc/sys/net counters. Per network namespace: on a bare box, the whole host.
	std::optional<bool> network = true;
	// systemd IPAccounting of our unit through `systemctl show`, which spawns a process.
	// Needs IPAccounting=yes in the unit.
	std::optional<bool> unitNet = true;
	// The /sockets sub page: one sock_diag dump of every TCP socket in the namespace. Off = 404.
	std::optional<bool> sockets = true;
	// Rows in each top list of the /sockets page
	std::optional<u32> socketTop = 10;
};

#endif // HOME_ROY_PUBLIC_DIGITALSPINE_RBK_THREAD_STATUSPAGECONF_H
