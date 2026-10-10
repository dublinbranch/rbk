#include "statusProbes.h"
#include "rbk/fmtExtra/includeMe.h"
#include "rbk/misc/escapeH.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <unordered_set>

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/inet_diag.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/sock_diag.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <spawn.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/timex.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

using std::string;
using std::string_view;

namespace {

// /proc and /sys files report size 0, so read until EOF instead of trusting stat()
string slurp(const string& path) {
	string    out;
	const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		return out;
	}
	char buf[4096];
	for (;;) {
		const auto n = ::read(fd, buf, sizeof(buf));
		if (n <= 0) {
			break;
		}
		out.append(buf, static_cast<size_t>(n));
	}
	::close(fd);
	return out;
}

string_view trim(string_view s) {
	const auto b = s.find_first_not_of(" \t\n");
	if (b == string_view::npos) {
		return {};
	}
	const auto e = s.find_last_not_of(" \t\n");
	return s.substr(b, e - b + 1);
}

bool isWs(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

std::vector<string_view> splitWs(string_view s) {
	std::vector<string_view> out;
	size_t                   i = 0;
	while (i < s.size()) {
		while (i < s.size() && isWs(s[i])) {
			++i;
		}
		const auto b = i;
		while (i < s.size() && !isWs(s[i])) {
			++i;
		}
		if (i > b) {
			out.push_back(s.substr(b, i - b));
		}
	}
	return out;
}

template <typename F>
void forEachLine(string_view text, F&& f) {
	size_t pos = 0;
	while (pos < text.size()) {
		auto nl = text.find('\n', pos);
		if (nl == string_view::npos) {
			nl = text.size();
		}
		f(text.substr(pos, nl - pos));
		pos = nl + 1;
	}
}

std::optional<u64> toU64(string_view s) {
	u64        v       = 0;
	const auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
	if (ec != std::errc{} || p == s.data()) {
		return std::nullopt;
	}
	return v;
}

i64 toI64(string_view s) {
	i64 v = 0;
	std::from_chars(s.data(), s.data() + s.size(), v);
	return v;
}

double toDouble(string_view s) {
	double v = 0;
	std::from_chars(s.data(), s.data() + s.size(), v);
	return v;
}

double dbl(u64 v) {
	return static_cast<double>(v);
}

// First number after `key` on the line that starts with it. Covers every flavour the kernel and
// systemctl print: "VmRSS:  12 kB", "rchar: 12", "oom_kill 0", "IPIngressBytes=12".
std::optional<u64> field(string_view text, string_view key) {
	std::optional<u64> res;
	bool               found = false;
	forEachLine(text, [&](string_view l) {
		if (found || l.size() <= key.size() || !l.starts_with(key)) {
			return;
		}
		const char sep = l[key.size()];
		if (sep != ':' && sep != ' ' && sep != '\t' && sep != '=') {
			return;
		}
		found = true;
		res   = toU64(trim(l.substr(key.size() + 1)));
	});
	return res;
}

string bytes(u64 b) {
	static constexpr std::array<const char*, 5> unit{"B", "KB", "MB", "GB", "TB"};
	auto                                        v = dbl(b);
	size_t                                      i = 0;
	while (v >= 1024.0 && i + 1 < unit.size()) {
		v /= 1024.0;
		++i;
	}
	if (i == 0) {
		return fmt::format("{} B", b);
	}
	return fmt::format("{:.1f} {}", v, unit[i]);
}

string duration(double sec) {
	const auto s = static_cast<u64>(sec);
	return fmt::format("{}d {:02}h {:02}m", s / 86400, (s % 86400) / 3600, (s % 3600) / 60);
}

double tvSec(const timeval& t) {
	return static_cast<double>(t.tv_sec) + static_cast<double>(t.tv_usec) / 1e6;
}

// cgroup "max" style limit: a number, or "max" for none
string limitText(string_view raw, bool asBytes) {
	raw = trim(raw);
	if (raw.empty()) {
		return "n/a";
	}
	if (raw == "max") {
		return "unlimited";
	}
	if (auto n = toU64(raw); n && asBytes) {
		return bytes(*n);
	}
	return string(raw);
}

/* posix_spawn rather than popen: no shell in the way, and the child closes every fd above stderr.
 * With 10K sockets open a popen() child would hold them all, so a close() on our side would not end
 * the connection until the child exits.
 */
string runCapture(std::vector<string> args) {
	int fds[2];
	if (::pipe2(fds, O_CLOEXEC) != 0) {
		return {};
	}
	posix_spawn_file_actions_t fa;
	posix_spawn_file_actions_init(&fa);
	posix_spawn_file_actions_adddup2(&fa, fds[1], STDOUT_FILENO);
	posix_spawn_file_actions_addopen(&fa, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
#ifdef __GLIBC__
#if __GLIBC_PREREQ(2, 34)
	posix_spawn_file_actions_addclosefrom_np(&fa, STDERR_FILENO + 1);
#endif
#endif
	std::vector<char*> argv;
	argv.reserve(args.size() + 1);
	for (auto& a : args) {
		argv.push_back(a.data());
	}
	argv.push_back(nullptr);

	pid_t     pid = 0;
	const int rc  = ::posix_spawnp(&pid, argv[0], &fa, nullptr, argv.data(), environ);
	posix_spawn_file_actions_destroy(&fa);
	::close(fds[1]);

	string out;
	if (rc == 0) {
		char buf[1024];
		for (;;) {
			const auto n = ::read(fds[0], buf, sizeof(buf));
			if (n > 0) {
				out.append(buf, static_cast<size_t>(n));
			} else if (n < 0 && errno == EINTR) {
				continue;
			} else {
				break;
			}
		}
		int status = 0;
		while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {
		}
	}
	::close(fds[0]);
	return out;
}

/* Calls f(fd, stat) for every fd we hold. FDSize in /proc/self/status bounds the fd table, and
 * fstat() of a number that is not open is a cheap EBADF, so probing them all costs about half of
 * listing /proc/self/fd, which builds a procfs entry per fd: 10K sockets is 6 ms against 12, and a
 * readlink() per fd would add 25 more. The table never shrinks, so a past peak keeps the probe
 * longer, at ~0.1 us per closed number.
 */
template <typename F>
void forEachFd(F&& f) {
	const auto fdSize = field(slurp("/proc/self/status"), "FDSize").value_or(0);
	for (u64 i = 0; i < fdSize; ++i) {
		const auto fd = static_cast<int>(i);
		struct stat st{};
		if (::fstat(fd, &st) == 0) {
			f(fd, st);
		}
	}
}

// Inode of every socket fd we hold: the only way to tell our sockets from the rest of the namespace.
// st_ino of a socket fd is the same number sock_diag reports as idiag_inode.
std::unordered_set<u32> ownSocketInodes() {
	std::unordered_set<u32> out;
	forEachFd([&](int, const struct stat& st) {
		if (S_ISSOCK(st.st_mode)) {
			out.insert(static_cast<u32>(st.st_ino));
		}
	});
	return out;
}

// /proc/net/snmp and /proc/net/netstat: per protocol a header line then a value line, "Tcp: A B" / "Tcp: 1 2"
std::map<string, i64, std::less<>> parsePairs(string_view text) {
	std::map<string, i64, std::less<>> out;
	std::vector<string_view>           lines;
	forEachLine(text, [&](string_view l) {
		if (!l.empty()) {
			lines.push_back(l);
		}
	});
	size_t i = 0;
	while (i + 1 < lines.size()) {
		const auto names = splitWs(lines[i]);
		const auto vals  = splitWs(lines[i + 1]);
		if (names.empty() || vals.empty() || names[0] != vals[0]) {
			++i;
			continue;
		}
		auto proto = names[0];
		if (proto.ends_with(':')) {
			proto.remove_suffix(1);
		}
		for (size_t j = 1; j < names.size() && j < vals.size(); ++j) {
			out[fmt::format("{}.{}", proto, names[j])] = toI64(vals[j]);
		}
		i += 2;
	}
	return out;
}

// "TCP: inuse 5 orphan 0 tw 0 alloc 7 mem 1" from /proc/net/sockstat
std::map<string, u64, std::less<>> sockstatMap(string_view text, string_view proto) {
	std::map<string, u64, std::less<>> m;
	forEachLine(text, [&](string_view l) {
		const auto f = splitWs(l);
		if (f.empty() || f[0] != proto) {
			return;
		}
		for (size_t i = 1; i + 1 < f.size(); i += 2) {
			m[string(f[i])] = toU64(f[i + 1]).value_or(0);
		}
	});
	return m;
}

struct DiagSock {
	u8                 family       = 0;
	u8                 state        = 0;
	u16                sport        = 0;
	u16                dport        = 0;
	u32                inode        = 0;
	u32                rq           = 0;
	u32                wq           = 0;
	std::array<u32, 4> dst          = {};
	bool               hasInfo      = false;
	u8                 retransmits  = 0; // consecutive RTO retransmits right now
	u32                rttUs        = 0;
	u32                lastRecvMs   = 0;
	u32                totalRetrans = 0;
	u32                unacked      = 0;
	u32                bufMem       = 0;
	u32                drops        = 0;
};

DiagSock parseDiag(const nlmsghdr* h) {
	const auto* base = reinterpret_cast<const char*>(h);
	const auto* m    = reinterpret_cast<const inet_diag_msg*>(base + NLMSG_HDRLEN);
	DiagSock    s;
	s.family = m->idiag_family;
	s.state  = m->idiag_state;
	s.sport  = ntohs(m->id.idiag_sport);
	s.dport  = ntohs(m->id.idiag_dport);
	s.inode  = m->idiag_inode;
	s.rq     = m->idiag_rqueue;
	s.wq     = m->idiag_wqueue;
	std::memcpy(s.dst.data(), m->id.idiag_dst, sizeof(s.dst));

	size_t off = NLMSG_LENGTH(sizeof(inet_diag_msg));
	while (off + sizeof(rtattr) <= h->nlmsg_len) {
		const auto* a = reinterpret_cast<const rtattr*>(base + off);
		if (a->rta_len < sizeof(rtattr) || off + a->rta_len > h->nlmsg_len) {
			break;
		}
		const auto*  data = reinterpret_cast<const char*>(a) + RTA_LENGTH(0);
		const size_t plen = a->rta_len - RTA_LENGTH(0);
		if (a->rta_type == INET_DIAG_INFO) {
			tcp_info ti{};
			std::memcpy(&ti, data, std::min(plen, sizeof(ti)));
			s.hasInfo      = true;
			s.retransmits  = ti.tcpi_retransmits;
			s.rttUs        = ti.tcpi_rtt;
			s.lastRecvMs   = ti.tcpi_last_data_recv;
			s.totalRetrans = ti.tcpi_total_retrans;
			s.unacked      = ti.tcpi_unacked;
		} else if (a->rta_type == INET_DIAG_SKMEMINFO) {
			std::array<u32, SK_MEMINFO_VARS> mem{};
			std::memcpy(mem.data(), data, std::min(plen, sizeof(mem)));
			s.bufMem = mem[SK_MEMINFO_RMEM_ALLOC] + mem[SK_MEMINFO_WMEM_QUEUED];
			s.drops  = mem[SK_MEMINFO_DROPS];
		}
		off += RTA_ALIGN(a->rta_len);
	}
	return s;
}

/* One NETLINK_SOCK_DIAG dump of every TCP socket of `family` in our network namespace, the way ss
 * does it. /proc/net/tcp renders the same table as text, which is what gets slow at 10K+.
 */
bool diagDump(u8 family, std::vector<DiagSock>& out, string& err) {
	const int fd = ::socket(AF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_SOCK_DIAG);
	if (fd < 0) {
		err = fmt::format("socket(NETLINK_SOCK_DIAG): {}", std::strerror(errno));
		return false;
	}
	timeval tv{2, 0};
	::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	struct {
		nlmsghdr         nlh;
		inet_diag_req_v2 req;
	} msg{};
	msg.nlh.nlmsg_len      = static_cast<u32>(sizeof(msg));
	msg.nlh.nlmsg_type     = SOCK_DIAG_BY_FAMILY;
	msg.nlh.nlmsg_flags    = static_cast<u16>(NLM_F_REQUEST | NLM_F_DUMP);
	msg.req.sdiag_family   = family;
	msg.req.sdiag_protocol = static_cast<u8>(IPPROTO_TCP);
	msg.req.idiag_states   = ~0u;
	msg.req.idiag_ext      = static_cast<u8>((1 << (INET_DIAG_INFO - 1)) | (1 << (INET_DIAG_SKMEMINFO - 1)));

	sockaddr_nl kernel{};
	kernel.nl_family = AF_NETLINK;
	if (::sendto(fd, &msg, sizeof(msg), 0, reinterpret_cast<sockaddr*>(&kernel), sizeof(kernel)) < 0) {
		err = fmt::format("sock_diag send: {}", std::strerror(errno));
		::close(fd);
		return false;
	}

	std::vector<char> buf(64 * 1024);
	bool              ok   = true;
	bool              done = false;
	while (!done) {
		const auto n = ::recv(fd, buf.data(), buf.size(), 0);
		if (n < 0 && errno == EINTR) {
			continue;
		}
		if (n <= 0) {
			if (n < 0) {
				err = fmt::format("sock_diag recv: {}", std::strerror(errno));
				ok  = false;
			}
			break;
		}
		const auto got = static_cast<size_t>(n);
		size_t     off = 0;
		while (off + sizeof(nlmsghdr) <= got) {
			const auto* h = reinterpret_cast<const nlmsghdr*>(buf.data() + off);
			if (h->nlmsg_len < sizeof(nlmsghdr) || off + h->nlmsg_len > got) {
				break;
			}
			if (h->nlmsg_type == NLMSG_DONE) {
				done = true;
				break;
			}
			if (h->nlmsg_type == NLMSG_ERROR) {
				int code = EPROTO; // too short to carry the errno
				if (h->nlmsg_len >= NLMSG_LENGTH(sizeof(nlmsgerr))) {
					code = -reinterpret_cast<const nlmsgerr*>(buf.data() + off + NLMSG_HDRLEN)->error;
				}
				// ipv6.disable=1 or no IPv6 module: no AF_INET6 diag handler, nothing to report
				if (family == AF_INET6 && (code == ENOENT || code == EAFNOSUPPORT)) {
					done = true;
					break;
				}
				err  = fmt::format("sock_diag: {}", std::strerror(code));
				ok   = false;
				done = true;
				break;
			}
			if (h->nlmsg_type == SOCK_DIAG_BY_FAMILY && h->nlmsg_len >= NLMSG_LENGTH(sizeof(inet_diag_msg))) {
				out.push_back(parseDiag(h));
			}
			off += NLMSG_ALIGN(h->nlmsg_len);
		}
	}
	::close(fd);
	return ok;
}

const char* tcpStateName(size_t s) {
	static constexpr std::array<const char*, 13> name{
	    "?", "ESTABLISHED", "SYN_SENT", "SYN_RECV", "FIN_WAIT1", "FIN_WAIT2", "TIME_WAIT",
	    "CLOSE", "CLOSE_WAIT", "LAST_ACK", "LISTEN", "CLOSING", "NEW_SYN_RECV"};
	return s < name.size() ? name[s] : "?";
}

string peer(const DiagSock& s) {
	char buf[INET6_ADDRSTRLEN]{};
	::inet_ntop(s.family, s.dst.data(), buf, sizeof(buf));
	if (s.family == AF_INET6) {
		return fmt::format("[{}]:{}", buf, s.dport);
	}
	return fmt::format("{}:{}", buf, s.dport);
}

struct Pct {
	bool   ok  = false;
	double p50 = 0;
	double p90 = 0;
	double p99 = 0;
	double max = 0;
};

Pct percentiles(std::vector<double> v) {
	Pct p;
	if (v.empty()) {
		return p;
	}
	std::sort(v.begin(), v.end());
	auto at = [&](double q) { return v[static_cast<size_t>(q * static_cast<double>(v.size() - 1))]; };
	p.ok    = true;
	p.p50   = at(0.5);
	p.p90   = at(0.9);
	p.p99   = at(0.99);
	p.max   = v.back();
	return p;
}

struct Group {
	u64                 n          = 0;
	u64                 sendQ      = 0;
	u64                 sendQMax   = 0;
	u64                 recvQ      = 0;
	u64                 recvQMax   = 0;
	u64                 retrans    = 0;
	u64                 retransNow = 0;
	u64                 unacked    = 0;
	u64                 mem        = 0;
	u64                 drops      = 0;
	u64                 est        = 0;
	std::vector<double> rttMs;
	std::vector<double> idleS;

	void add(const DiagSock& s) {
		++n;
		sendQ += s.wq;
		sendQMax = std::max<u64>(sendQMax, s.wq);
		recvQ += s.rq;
		recvQMax = std::max<u64>(recvQMax, s.rq);
		mem += s.bufMem;
		drops += s.drops;
		if (s.state == TCP_ESTABLISHED) {
			++est;
		}
		if (!s.hasInfo) {
			return;
		}
		retrans += s.totalRetrans;
		retransNow += s.retransmits > 0;
		unacked += s.unacked;
		if (s.state == TCP_ESTABLISHED) {
			rttMs.push_back(s.rttUs / 1000.0);
			idleS.push_back(s.lastRecvMs / 1000.0);
		}
	}
};

// 1234567 -> "1 234 567", narrow no-break space so it never wraps
string grouped(u64 v) {
	const auto s = std::to_string(v);
	string     out;
	for (size_t i = 0; i < s.size(); ++i) {
		if (i > 0 && (s.size() - i) % 3 == 0) {
			out += "&#8239;";
		}
		out += s[i];
	}
	return out;
}

string unitSlot(string_view unit) {
	return fmt::format("<span class=\"u\">{}</span>", unit);
}

// Header row of a data table
string headRow(std::initializer_list<string_view> cols) {
	string out = "<tr class=\"hd\">";
	for (auto c : cols) {
		out += fmt::format("<th>{}</th>", c);
	}
	return out + "</tr>\n";
}

template <typename Key>
string topTable(string_view title, std::vector<const DiagSock*> v, u32 top, Key key) {
	std::erase_if(v, [&](const DiagSock* s) { return key(*s) == 0; });
	const auto n = std::min<size_t>(top, v.size());
	std::partial_sort(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(n), v.end(),
	                  [&](const DiagSock* a, const DiagSock* b) { return key(*a) > key(*b); });
	string out = fmt::format("<section><h2>{}</h2>\n<p class=\"note\">{} sockets with a non zero value</p>\n<table class=\"t\">\n", title, v.size());
	out += headRow({"peer", "local port", "state", "Send-Q", "Recv-Q", "RTT ms", "idle rx s", "retrans"});
	for (size_t i = 0; i < n; ++i) {
		const auto& s = *v[i];
		out += fmt::format("<tr><td>{}</td>{}<td>{}</td>{}{}{}{}{}</tr>\n", peer(s), statusProbes::td(fmt::format("{}", s.sport)),
		                   tcpStateName(s.state), statusProbes::td(statusProbes::nBytes(s.wq)),
		                   statusProbes::td(statusProbes::nBytes(s.rq)), statusProbes::td(statusProbes::nFix(s.rttUs / 1000.0, 2)),
		                   statusProbes::td(statusProbes::nFix(s.lastRecvMs / 1000.0, 1)),
		                   statusProbes::td(statusProbes::nInt(s.totalRetrans)));
	}
	out += "</table></section>\n";
	return out;
}

string unitNetRows() {
	using namespace statusProbes;
	const auto cgRaw = slurp("/proc/self/cgroup");
	string     unit;
	forEachLine(cgRaw, [&](string_view l) {
		if (!l.starts_with("0::")) {
			return;
		}
		const auto p     = trim(l.substr(3));
		const auto slash = p.rfind('/');
		const auto last  = slash == string_view::npos ? p : p.substr(slash + 1);
		if (last.ends_with(".service")) {
			unit = string(last);
		}
	});
	if (unit.empty()) {
		return rowT("Unit", "not running as a systemd service");
	}
	// timeout: a stuck D-Bus would otherwise hold this thread for systemctl's 25 s. -k: KILL if TERM is not enough
	const auto text = runCapture({"timeout", "-k", "1", "2", "systemctl", "show", unit, "-p", "IPIngressBytes", "-p",
	                              "IPEgressBytes", "-p", "IPIngressPackets", "-p", "IPEgressPackets"});
	auto       v    = [&](string_view k) -> std::optional<u64> {
        const auto r = field(text, k);
        if (r && *r == std::numeric_limits<u64>::max()) {
            return std::nullopt; // older systemd prints UINT64_MAX for "no data"
        }
        return r;
	};
	const auto inB  = v("IPIngressBytes");
	const auto outB = v("IPEgressBytes");
	string     r    = rowT("Unit", escapeH(unit));
	if (!inB || !outB) {
		return r + rowT("Traffic", "no data (IPAccounting=yes missing, or the kernel has no cgroup BPF)");
	}
	r += rowN("In", nBytes(*inB), "since unit start, loopback included");
	r += rowN("In packets", kInt(v("IPIngressPackets").value_or(0)));
	r += rowN("Out", nBytes(*outB));
	r += rowN("Out packets", kInt(v("IPEgressPackets").value_or(0)));
	return r;
}

} // namespace

namespace statusProbes {

string nInt(u64 v) {
	return grouped(v);
}

string nFix(double v, int decimals, string_view unit) {
	auto s = fmt::format("{:.{}f}", v, decimals);
	if (!unit.empty()) {
		s += unitSlot(unit);
	}
	return s;
}

string nBytes(u64 b) {
	static constexpr std::array<const char*, 5> unit{"B", "KB", "MB", "GB", "TB"};
	auto                                        v = dbl(b);
	size_t                                      i = 0;
	while (v >= 1024.0 && i + 1 < unit.size()) {
		v /= 1024.0;
		++i;
	}
	if (i == 0) {
		return grouped(b) + "<span class=\"h\">.0</span>" + unitSlot("B");
	}
	return fmt::format("{:.1f}", v) + unitSlot(unit[i]);
}

string kInt(u64 v) {
	return grouped(v) + "<span class=\"h\">.0</span>" + unitSlot("");
}

string kFix(double v, string_view unit) {
	return fmt::format("{:.1f}", v) + unitSlot(unit);
}

string warn(string_view msg) {
	return fmt::format("<span class=\"warn\">{}</span>", msg);
}

string td(string_view num) {
	return fmt::format("<td class=\"n\">{}</td>", num);
}

string rowN(string_view label, string_view num, string_view note) {
	return fmt::format("<tr><th>{}</th><td class=\"n\">{}</td><td class=\"note\">{}</td></tr>\n", label, num, note);
}

string rowT(string_view label, string_view text) {
	return fmt::format("<tr><th>{}</th><td colspan=\"2\">{}</td></tr>\n", label, text);
}

string kvSection(string_view id, string_view title, string_view rows) {
	return fmt::format("<section{}><h2>{}</h2>\n<table class=\"kv\">\n{}</table></section>\n",
	                   id.empty() ? string() : fmt::format(" id=\"{}\"", id), title, rows);
}

string processRows(double upSec, size_t poolSize) {
	struct rusage ru{};
	getrusage(RUSAGE_SELF, &ru);
	const double user  = tvSec(ru.ru_utime);
	const double sys   = tvSec(ru.ru_stime);
	const double avg   = upSec > 0 ? (user + sys) / upSec * 100.0 : 0.0;
	const auto   cores = sysconf(_SC_NPROCESSORS_ONLN);
	auto         ul    = [](long v) { return static_cast<u64>(std::max(0L, v)); };

	string r;
	r += rowN("CPU user", kFix(user, "s"));
	r += rowN("CPU system", kFix(sys, "s"));
	r += rowN("CPU average", kFix(avg, "%"), fmt::format("of one core since start, {} cores", cores));

	const auto st = slurp("/proc/self/status");
	if (!st.empty()) {
		auto       kb   = [&](string_view k) { return field(st, k).value_or(0) * 1024; };
		const auto swap = kb("VmSwap");
		r += rowN("RSS", nBytes(kb("VmRSS")),
		          fmt::format("anon {} · file {} · shmem {}", bytes(kb("RssAnon")), bytes(kb("RssFile")), bytes(kb("RssShmem"))));
		r += rowN("RSS peak", nBytes(kb("VmHWM")));
		r += rowN("Virtual", nBytes(kb("VmSize")));
		r += rowN("Swapped out", nBytes(swap), swap > 0 ? warn("partly swapped out") : string());
		r += rowN("OS threads", kInt(field(st, "Threads").value_or(0)), fmt::format("{} in the status pool", poolSize));
	}
	r += rowN("Ctx sw voluntary", kInt(ul(ru.ru_nvcsw)), "gave up the core: I/O, locks, sleep");
	r += rowN("Ctx sw involuntary", kInt(ul(ru.ru_nivcsw)), "preempted");
	r += rowN("Page faults major", kInt(ul(ru.ru_majflt)), "had to read from disk");
	r += rowN("Page faults minor", kInt(ul(ru.ru_minflt)));

	const auto io = slurp("/proc/self/io");
	if (!io.empty()) {
		auto v = [&](string_view k) { return field(io, k).value_or(0); };
		r += rowN("Disk read", nBytes(v("read_bytes")));
		r += rowN("Disk write", nBytes(v("write_bytes")));
		r += rowN("Disk write cancelled", nBytes(v("cancelled_write_bytes")));
		r += rowN("All read", nBytes(v("rchar")), fmt::format("{} calls, files + sockets + pipes", grouped(v("syscr"))));
		r += rowN("All write", nBytes(v("wchar")), fmt::format("{} calls", grouped(v("syscw"))));
	}

	const auto oom = toU64(trim(slurp("/proc/self/oom_score")));
	if (oom) {
		r += rowN("OOM score", kInt(*oom), fmt::format("adj {}", trim(slurp("/proc/self/oom_score_adj"))));
	}
	return r;
}

FdCount fdCount(bool kinds) {
	FdCount               res;
	u64                   nSock  = 0;
	u64                   nPipe  = 0;
	u64                   nFile  = 0;
	u64                   nDev   = 0;
	u64                   nOther = 0;
	std::map<string, u64> anon;
	char                  buf[256];
	forEachFd([&](int fd, const struct stat& st) {
		++res.open;
		if (!kinds) {
			return;
		}
		if (S_ISSOCK(st.st_mode)) {
			++nSock;
			return;
		}
		if (S_ISFIFO(st.st_mode)) {
			++nPipe;
			return;
		}
		// The few left: only the link text tells a file from a device from an anon inode (epoll, eventfd...)
		const auto n = ::readlink(fmt::format("/proc/self/fd/{}", fd).c_str(), buf, sizeof(buf));
		if (n <= 0) {
			return; // closed meanwhile
		}
		const string_view l(buf, static_cast<size_t>(n));
		if (l.starts_with("anon_inode:")) {
			auto k = l.substr(11);
			if (k.starts_with('[') && k.ends_with(']')) {
				k = k.substr(1, k.size() - 2);
			}
			++anon[string(k)];
		} else if (l.starts_with("/dev/")) {
			++nDev;
		} else if (l.starts_with('/')) {
			++nFile;
		} else {
			++nOther;
		}
	});
	if (!kinds) {
		return res;
	}

	auto& out = res.kinds;
	out       = fmt::format("socket {} · pipe {} · file {} · dev {}", nSock, nPipe, nFile, nDev);
	std::vector<std::pair<u64, string>> byCount;
	for (const auto& [k, c] : anon) {
		byCount.emplace_back(c, k);
	}
	std::sort(byCount.rbegin(), byCount.rend());
	for (const auto& [c, k] : byCount) {
		out += fmt::format(" · {} {}", escapeH(k), c);
	}
	if (nOther) {
		out += fmt::format(" · other {}", nOther);
	}
	return res;
}

string hostSection(const std::vector<string>& diskPaths, string_view leadRows) {
	string host(leadRows);

	struct utsname u{};
	if (::uname(&u) == 0) {
		host += rowT("Kernel", fmt::format("{} {} {}", u.sysname, u.release, u.machine));
	}
	const auto upRaw = slurp("/proc/uptime");
	if (const auto f = splitWs(upRaw); !f.empty()) {
		host += rowT("Host uptime", duration(toDouble(f[0])));
	}
	{
		struct timex tx{};
		const int    state = ::adjtimex(&tx);
		const auto   now   = std::time(nullptr);
		std::tm      tm{};
		gmtime_r(&now, &tm);
		char ts[32]{};
		std::strftime(ts, sizeof(ts), "%F %T", &tm);
		if (state == -1) {
			host += rowT("Clock", fmt::format("{} UTC · adjtimex failed: {}", ts, std::strerror(errno)));
		} else if (state == TIME_ERROR || (tx.status & STA_UNSYNC)) {
			host += rowT("Clock", fmt::format("{} UTC · {}", ts, warn("NOT synchronised, no NTP lock, timestamps can drift")));
		} else {
			host += rowT("Clock", fmt::format("{} UTC · synchronised, est error {:.1f} ms, max error {:.1f} ms", ts,
			                                  static_cast<double>(tx.esterror) / 1000.0, static_cast<double>(tx.maxerror) / 1000.0));
		}
	}

	// cgroup v2 only, "0::/system.slice/digitalSpine.service"
	const auto cgRaw = slurp("/proc/self/cgroup");
	string     cg;
	forEachLine(cgRaw, [&](string_view l) {
		if (l.starts_with("0::")) {
			cg = string(trim(l.substr(3)));
		}
	});
	string cgRows;
	if (cg.empty()) {
		host += rowT("Cgroup", "not on cgroup v2");
	} else {
		host += rowT("Cgroup", escapeH(cg));
		const string base = "/sys/fs/cgroup" + cg + "/";

		if (const auto cur = toU64(trim(slurp(base + "memory.current")))) {
			const auto events = slurp(base + "memory.events");
			auto       ev     = [&](string_view k) { return field(events, k).value_or(0); };
			// Page cache of the files we write counts here too, so this is above the RSS
			cgRows += rowN("Memory", nBytes(*cur), "page cache of our files included");
			if (const auto p = toU64(trim(slurp(base + "memory.peak")))) {
				cgRows += rowN("Memory peak", nBytes(*p));
			}
			cgRows += rowT("Memory high / max", fmt::format("{} / {}", limitText(slurp(base + "memory.high"), true),
			                                                 limitText(slurp(base + "memory.max"), true)));
			const auto oomKill = ev("oom_kill");
			cgRows += rowT("Memory events", fmt::format("high {} · max {} · oom {} · oom_kill {}{}", ev("high"), ev("max"), ev("oom"),
			                                            oomKill, oomKill ? " " + warn("the kernel killed in this cgroup") : string()));
		}

		const auto cpuStat = slurp(base + "cpu.stat");
		if (!cpuStat.empty()) {
			auto       cs        = [&](string_view k) { return field(cpuStat, k).value_or(0); };
			const auto cpuMaxRaw = slurp(base + "cpu.max");
			const auto cm        = splitWs(cpuMaxRaw);
			string     cpuLimit  = "n/a";
			if (cm.size() >= 2 && cm[0] == "max") {
				cpuLimit = "unlimited";
			} else if (cm.size() >= 2) {
				const auto period = toU64(cm[1]).value_or(0);
				cpuLimit          = period ? fmt::format("{:.2f} cores", dbl(toU64(cm[0]).value_or(0)) / dbl(period)) : string(cm[0]);
			}
			const auto throttled = cs("nr_throttled");
			cgRows += rowN("CPU used", kFix(dbl(cs("usage_usec")) / 1e6, "s"));
			cgRows += rowT("CPU limit", cpuLimit);
			cgRows += rowN("CPU throttled", kInt(throttled),
			               fmt::format("of {} periods, {:.1f} s{}", cs("nr_periods"), dbl(cs("throttled_usec")) / 1e6,
			                           throttled ? " " + warn("CPU quota hit") : string()));
		}

		if (const auto pids = toU64(trim(slurp(base + "pids.current")))) {
			cgRows += rowN("Pids", kInt(*pids), "max " + limitText(slurp(base + "pids.max"), false));
		}
	}

	// Load and PSI: three windows each, two decimals
	string     lp;
	const auto loadRaw = slurp("/proc/loadavg");
	if (const auto f = splitWs(loadRaw); f.size() >= 4) {
		lp += headRow({"", "1 min", "5 min", "15 min", ""});
		lp += fmt::format("<tr><th>Load</th>{}{}{}<td class=\"note\">{} cores, runnable / tasks {}</td></tr>\n",
		                  td(nFix(toDouble(f[0]), 2)), td(nFix(toDouble(f[1]), 2)), td(nFix(toDouble(f[2]), 2)),
		                  sysconf(_SC_NPROCESSORS_ONLN), f[3]);
	}
	bool anyPsi = false;
	for (const char* res : {"cpu", "memory", "io"}) {
		const auto text = slurp(fmt::format("/proc/pressure/{}", res));
		forEachLine(text, [&](string_view l) {
			const auto parts = splitWs(l);
			if (parts.size() < 4) {
				return;
			}
			if (!anyPsi) {
				lp += headRow({"PSI, % of time stalled", "10 s", "60 s", "300 s", ""});
				anyPsi = true;
			}
			auto val = [](string_view kv) {
				const auto eq = kv.find('=');
				return eq == string_view::npos ? 0.0 : toDouble(kv.substr(eq + 1));
			};
			lp += fmt::format("<tr><th>{} {}</th>{}{}{}<td class=\"note\">{}</td></tr>\n", res, parts[0],
			                  td(nFix(val(parts[1]), 2)), td(nFix(val(parts[2]), 2)), td(nFix(val(parts[3]), 2)),
			                  parts[0] == "some" ? "at least one task waited" : "all tasks waited");
		});
	}
	if (!anyPsi) {
		lp += "<tr><th>PSI</th><td colspan=\"4\">unavailable (kernel without CONFIG_PSI, or booted with psi=0)</td></tr>\n";
	}

	string disks;
	for (const auto& p : diskPaths) {
		struct statvfs v{};
		if (::statvfs(p.c_str(), &v) != 0) {
			disks += fmt::format("<tr><td>{}</td><td colspan=\"5\">unavailable: {}</td></tr>\n", escapeH(p), std::strerror(errno));
			continue;
		}
		const u64       total = static_cast<u64>(v.f_blocks) * v.f_frsize;
		const u64       avail = static_cast<u64>(v.f_bavail) * v.f_frsize;
		const double    pct   = total ? 100.0 * dbl(avail) / dbl(total) : 0.0;
		std::error_code ec;
		const auto      canon = std::filesystem::canonical(p, ec);
		// btrfs and friends report 0 inodes: nothing to run out of
		const string inodes = v.f_files > 0 ? td(nFix(100.0 * dbl(v.f_favail) / dbl(v.f_files), 1, "%")) : "<td class=\"n\">-</td>";
		disks += fmt::format("<tr><td>{}</td>{}{}{}{}<td class=\"note\">{}</td></tr>\n", escapeH(ec ? p : canon.string()),
		                     td(nBytes(avail)), td(nBytes(total)), td(nFix(pct, 1, "%")), inodes,
		                     pct < 10 ? warn("under 10% free") : string());
	}

	string out = "<div class=\"grid\">\n" + kvSection("host", "Host", host);
	out += "<section><h2>Load and pressure</h2>\n<table class=\"t\">\n" + lp + "</table></section>\n";
	if (!cgRows.empty()) {
		out += kvSection("", "Cgroup", cgRows);
	}
	if (!disks.empty()) {
		out += "<section><h2>Disks</h2>\n<table class=\"t\">\n" +
		       headRow({"path", "free", "total", "free", "inodes free", ""}) + disks + "</table></section>\n";
	}
	out += "</div>\n";
	return out;
}

string networkSection(bool counters, bool unitNet, string_view socketsHref) {
	string out = "<section id=\"network\"><h2>Network</h2>\n<p class=\"note\">Counters of the whole network namespace since boot (on a bare box, the whole host)";
	if (!socketsHref.empty()) {
		out += fmt::format(" · <a href=\"{}\">TCP sockets of this process</a>", socketsHref);
	}
	out += "</p>\n";

	string grid;
	if (counters) {
		// Counters since the interface came up. lo is the local DB traffic, the rest is clients.
		const auto dev     = slurp("/proc/net/dev");
		int        lineNo  = 0;
		size_t     shown   = 0;
		size_t     skipped = 0;
		string     ifs;
		forEachLine(dev, [&](string_view l) {
			if (lineNo++ < 2) {
				return; // two header lines
			}
			const auto colon = l.find(':');
			if (colon == string_view::npos) {
				return;
			}
			const auto name = trim(l.substr(0, colon));
			const auto f    = splitWs(l.substr(colon + 1));
			if (f.size() < 12) {
				return;
			}
			auto       c   = [&](size_t i) { return toU64(f[i]).value_or(0); };
			const auto rxB = c(0);
			const auto txB = c(8);
			if (rxB == 0 && txB == 0) {
				return;
			}
			if (shown == 12) {
				++skipped;
				return;
			}
			++shown;
			ifs += fmt::format("<tr><td>{}</td>{}{}{}{}{}{}{}{}</tr>\n", escapeH(name), td(nBytes(rxB)), td(nInt(c(1))),
			                   td(nInt(c(2))), td(nInt(c(3))), td(nBytes(txB)), td(nInt(c(9))), td(nInt(c(10))), td(nInt(c(11))));
		});
		if (skipped) {
			ifs += fmt::format("<tr><td colspan=\"9\">... {} more interfaces with traffic</td></tr>\n", skipped);
		}
		if (!ifs.empty()) {
			out += "<section><table class=\"t\">\n" +
			       headRow({"interface", "rx", "rx packets", "rx err", "rx drop", "tx", "tx packets", "tx err", "tx drop"}) + ifs +
			       "</table></section>\n";
		}

		auto       m       = parsePairs(slurp("/proc/net/snmp"));
		const auto netstat = parsePairs(slurp("/proc/net/netstat"));
		m.insert(netstat.begin(), netstat.end());
		auto g = [&](string_view k) {
			const auto it = m.find(k);
			return it == m.end() ? u64{0} : static_cast<u64>(std::max<i64>(0, it->second));
		};
		if (m.contains("Tcp.InSegs")) {
			const auto outSegs = g("Tcp.OutSegs");
			const auto retrans = g("Tcp.RetransSegs");
			string     r;
			r += rowN("Established now", kInt(g("Tcp.CurrEstab")));
			r += rowN("Opens active", kInt(g("Tcp.ActiveOpens")), "we connected");
			r += rowN("Opens passive", kInt(g("Tcp.PassiveOpens")), "accepted");
			r += rowN("Attempt fails", kInt(g("Tcp.AttemptFails")));
			r += rowN("Estab resets", kInt(g("Tcp.EstabResets")));
			r += rowN("Segments in", kInt(g("Tcp.InSegs")));
			r += rowN("Segments out", kInt(outSegs));
			r += rowN("Retransmitted", kInt(retrans),
			          fmt::format("{:.3f}% of out", outSegs ? 100.0 * dbl(retrans) / dbl(outSegs) : 0.0));
			r += rowN("In errors", kInt(g("Tcp.InErrs")));
			r += rowN("Out resets", kInt(g("Tcp.OutRsts")));
			grid += kvSection("", "TCP", r);
		}
		{
			const auto overflows = g("TcpExt.ListenOverflows");
			const auto drops     = g("TcpExt.ListenDrops");
			const auto dropWarn  = warn("dropped before accept()");
			string     r;
			r += rowN("Listen overflows", kInt(overflows), overflows ? dropWarn : string());
			r += rowN("Listen drops", kInt(drops), drops ? dropWarn : string());
			r += rowN("Syncookies sent", kInt(g("TcpExt.SyncookiesSent")));
			r += rowN("Timeouts", kInt(g("TcpExt.TCPTimeouts")));
			r += rowN("SYN retrans", kInt(g("TcpExt.TCPSynRetrans")));
			r += rowN("Abort on timeout", kInt(g("TcpExt.TCPAbortOnTimeout")));
			r += rowN("Abort on memory", kInt(g("TcpExt.TCPAbortOnMemory")));
			r += rowN("Memory pressure", kInt(g("TcpExt.TCPMemoryPressures")));
			r += rowN("Backlog drop", kInt(g("TcpExt.TCPBacklogDrop")));
			r += rowN("Pruned", kInt(g("TcpExt.PruneCalled")));
			grid += kvSection("", "TCP trouble", r);
		}

		const auto ss   = slurp("/proc/net/sockstat");
		auto       tcp  = sockstatMap(ss, "TCP:");
		auto       tcp6 = sockstatMap(slurp("/proc/net/sockstat6"), "TCP6:");
		auto       sock = sockstatMap(ss, "sockets:");
		string     r;
		if (!tcp.empty()) {
			// inuse is per family, alloc / orphan / tw / mem are shared by v4 and v6
			r += rowN("TCP in use v4", kInt(tcp["inuse"]));
			r += rowN("TCP in use v6", kInt(tcp6["inuse"]));
			r += rowN("TCP allocated", kInt(tcp["alloc"]), "v4 + v6");
			r += rowN("Orphans", kInt(tcp["orphan"]), "closed, still flushing");
			r += rowN("TIME_WAIT", kInt(tcp["tw"]));
			r += rowN("Sockets used", kInt(sock["used"]), "every family");
			const auto page      = static_cast<u64>(sysconf(_SC_PAGESIZE));
			const auto memPages  = tcp["mem"];
			const auto tcpMemRaw = slurp("/proc/sys/net/ipv4/tcp_mem");
			const auto tm        = splitWs(tcpMemRaw);
			string     note;
			if (tm.size() >= 3) {
				const auto atPressure = toU64(tm[1]).value_or(0);
				note = fmt::format("pressure at {}, max {}", bytes(atPressure * page), bytes(toU64(tm[2]).value_or(0) * page));
				if (atPressure && memPages >= atPressure) {
					note += " " + warn("over the pressure threshold, the kernel is shrinking buffers");
				}
			}
			r += rowN("TCP memory", nBytes(memPages * page), note);
		}
		r += rowN("somaxconn", kInt(toU64(trim(slurp("/proc/sys/net/core/somaxconn"))).value_or(0)));
		r += rowN("SYN backlog", kInt(toU64(trim(slurp("/proc/sys/net/ipv4/tcp_max_syn_backlog"))).value_or(0)));
		const auto ctCount = toU64(trim(slurp("/proc/sys/net/netfilter/nf_conntrack_count")));
		const auto ctMax   = toU64(trim(slurp("/proc/sys/net/netfilter/nf_conntrack_max")));
		if (ctCount && ctMax && *ctMax) {
			const double pct = 100.0 * dbl(*ctCount) / dbl(*ctMax);
			r += rowN("Conntrack", kInt(*ctCount),
			          fmt::format("of {} ({:.1f}%){}", grouped(*ctMax), pct, pct > 80 ? " " + warn("when full the kernel drops new connections") : string()));
		} else {
			r += rowT("Conntrack", "not loaded");
		}
		grid += kvSection("", "Sockets", r);
	}
	if (unitNet) {
		grid += kvSection("", "Unit traffic", unitNetRows());
	}
	if (!grid.empty()) {
		out += "<div class=\"grid\">\n" + grid + "</div>\n";
	}
	out += "</section>\n";
	return out;
}

ThreadKernel threadKernel(int tid) {
	ThreadKernel k;
	if (tid <= 0) {
		return k;
	}
	const auto base  = fmt::format("/proc/self/task/{}/", tid);
	const auto stat  = slurp(base + "stat");
	const auto close = stat.rfind(')'); // comm can hold spaces and parens, the last ')' ends it
	const auto open  = stat.find('(');
	if (close == string::npos || open == string::npos || open > close) {
		return k;
	}
	k.comm = stat.substr(open + 1, close - open - 1);
	// fields from 3 (state) on, so field N is at N - 3
	const auto f = splitWs(string_view(stat).substr(close + 1));
	if (f.size() < 37) {
		return k;
	}
	static const double tick = static_cast<double>(sysconf(_SC_CLK_TCK));
	k.state                  = f[0].empty() ? '?' : f[0][0];
	k.cpuSec                 = dbl(toU64(f[11]).value_or(0) + toU64(f[12]).value_or(0)) / tick;
	k.core                   = static_cast<int>(toI64(f[36]));
	k.ok                     = true;

	// ns on cpu, ns runnable but waiting, timeslices. Finer than the clock ticks of stat.
	const auto schedRaw = slurp(base + "schedstat");
	if (const auto ss = splitWs(schedRaw); ss.size() >= 2) {
		k.cpuSec  = dbl(toU64(ss[0]).value_or(0)) / 1e9;
		k.waitSec = dbl(toU64(ss[1]).value_or(0)) / 1e9;
	}
	return k;
}

string kernelCells(const ThreadKernel& k) {
	if (!k.ok) {
		return "<td>-</td><td class=\"n\">-</td><td class=\"n\">-</td><td class=\"n\">-</td>";
	}
	// D = blocked inside the kernel (disk, NFS, a stuck driver), never good on an HttpHandler
	const string state = k.state == 'D' ? warn("D") : string(1, k.state);
	return fmt::format("<td>{}</td>{}{}{}", state, td(fmt::format("{}", k.core)), td(nFix(k.cpuSec, 2)),
	                   k.waitSec < 0 ? td("-") : td(nFix(k.waitSec, 2)));
}

string otherThreadsTable(const std::vector<int>& poolTids) {
	DIR* d = ::opendir("/proc/self/task");
	if (d == nullptr) {
		return {};
	}
	std::vector<std::pair<int, ThreadKernel>> rows;
	while (auto* e = ::readdir(d)) {
		const auto id = toU64(e->d_name);
		if (!id) {
			continue;
		}
		const auto tid = static_cast<int>(*id);
		if (std::find(poolTids.begin(), poolTids.end(), tid) != poolTids.end()) {
			continue;
		}
		if (auto k = threadKernel(tid); k.ok) {
			rows.emplace_back(tid, std::move(k));
		}
	}
	::closedir(d);
	std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.second.cpuSec > b.second.cpuSec; });

	string out = fmt::format("<section><h2>Threads outside the pool</h2>\n<p class=\"note\">{} threads, by CPU time. cpu and "
	                         "rq wait (runnable, waiting for a core) in s since the thread started</p>\n<table class=\"t\">\n",
	                         rows.size());
	out += headRow({"tid", "name", "kstate", "core", "cpu s", "rq wait s"});
	constexpr size_t maxRows = 100;
	for (size_t i = 0; i < rows.size() && i < maxRows; ++i) {
		const auto& [tid, k] = rows[i];
		out += fmt::format("<tr>{}<td>{}</td>{}</tr>\n", td(fmt::format("{}", tid)), escapeH(k.comm), kernelCells(k));
	}
	if (rows.size() > maxRows) {
		out += fmt::format("<tr><td colspan=\"6\">... {} more</td></tr>\n", rows.size() - maxRows);
	}
	out += "</table></section>\n";
	return out;
}

string socketsBody(u32 top) {
	using clock    = std::chrono::steady_clock;
	const auto t0  = clock::now();
	const auto own = ownSocketInodes();
	const auto t1  = clock::now();

	std::vector<DiagSock> all;
	all.reserve(own.size() + 256);
	string     err;
	const bool ok4 = diagDump(AF_INET, all, err);
	const bool ok6 = diagDump(AF_INET6, all, err);
	const auto t2  = clock::now();
	if (!ok4 && !ok6) {
		return fmt::format("<p>{}</p>\n", warn(escapeH(err)));
	}

	std::set<u16> listenPorts;
	for (const auto& s : all) {
		if (s.state == TCP_LISTEN && own.contains(s.inode)) {
			listenPorts.insert(s.sport);
		}
	}

	std::array<u64, 13>                         byState{};
	u64                                         nsTimeWait  = 0;
	u64                                         ourTimeWait = 0;
	u64                                         ourSynRecv  = 0;
	u64                                         ourOrphans  = 0;
	u64                                         notAccepted = 0;
	std::vector<const DiagSock*>                listeners;
	std::vector<const DiagSock*>                inbound;
	std::vector<const DiagSock*>                conns;
	std::map<u16, std::vector<const DiagSock*>> outbound;
	for (const auto& s : all) {
		// TIME_WAIT and handshakes in flight have no fd: attribute them by port
		if (s.state == TCP_TIME_WAIT) {
			++nsTimeWait;
			ourTimeWait += listenPorts.contains(s.sport);
			continue;
		}
		if (s.inode == 0 && s.state == TCP_SYN_RECV) {
			ourSynRecv += listenPorts.contains(s.sport);
			continue;
		}
		/* No fd either, so sock_diag reports inode 0, and on our listen ports they are ours:
		 * - after our close() a socket still flushing its Send-Q or its FIN (FIN_WAIT1 / 2, LAST_ACK,
		 *   CLOSING): a client we dropped
		 * - a connection still in the accept queue (ESTABLISHED, or CLOSE_WAIT if the peer gave up)
		 * Outbound ones sit on an ephemeral port, no way to tell them from another process's: left out.
		 */
		if (s.inode == 0) {
			if (!listenPorts.contains(s.sport)) {
				continue;
			}
			switch (s.state) {
			case TCP_FIN_WAIT1:
			case TCP_FIN_WAIT2:
			case TCP_LAST_ACK:
			case TCP_CLOSING:
				++ourOrphans;
				break;
			default:
				++notAccepted;
			}
		} else if (!own.contains(s.inode)) {
			continue;
		}
		if (s.state < byState.size()) {
			++byState[s.state];
		}
		if (s.state == TCP_LISTEN) {
			listeners.push_back(&s);
			continue;
		}
		conns.push_back(&s);
		if (listenPorts.contains(s.sport)) {
			inbound.push_back(&s);
		} else {
			outbound[s.dport].push_back(&s);
		}
	}

	u64 ownTotal = 0;
	for (auto c : byState) {
		ownTotal += c;
	}

	string sum;
	sum += rowN("Own TCP sockets", kInt(ownTotal));
	sum += rowN("In the namespace", kInt(all.size()), "every process");
	sum += rowN("TIME_WAIT", kInt(nsTimeWait), "in the namespace, no owner");
	sum += rowN("Ours in SYN_RECV", kInt(ourSynRecv), "handshakes in flight");
	sum += rowN("Ours in TIME_WAIT", kInt(ourTimeWait), "connections we closed first");
	sum += rowN("Closed, still flushing", kInt(ourOrphans),
	            ourOrphans ? warn("no fd, Send-Q or FIN not acked yet, holds kernel memory") : string("no fd, counted in the states"));
	sum += rowN("Not accepted yet", kInt(notAccepted), "no fd, counted in the states");

	string states;
	for (size_t i = 0; i < byState.size(); ++i) {
		if (byState[i]) {
			states += rowN(tcpStateName(i), kInt(byState[i]),
			               i == TCP_CLOSE_WAIT ? warn("the peer closed and we never did") : string());
		}
	}

	string lst;
	for (const auto* l : listeners) {
		const bool nearFull = l->wq && l->rq * 10 >= l->wq * 9;
		lst += fmt::format("<tr>{}{}{}<td class=\"note\">{}</td></tr>\n", td(fmt::format("{}", l->sport)), td(nInt(l->rq)),
		                   td(nInt(l->wq)), nearFull ? warn("nearly full") : string());
	}

	string out = "<div class=\"grid\">\n" + kvSection("", "Summary", sum) + kvSection("", "Own sockets by state", states);
	out += "<section><h2>Listeners</h2>\n<table class=\"t\">\n" + headRow({"port", "accept queue", "backlog", ""}) + lst +
	       "</table></section>\n</div>\n";

	Group in;
	for (const auto* s : inbound) {
		in.add(*s);
	}
	string inRows;
	inRows += rowN("Connections", kInt(in.n));
	inRows += rowN("Established", kInt(in.est));
	inRows += rowN("Send-Q", nBytes(in.sendQ), fmt::format("max {}", bytes(in.sendQMax)));
	inRows += rowN("Recv-Q", nBytes(in.recvQ), fmt::format("max {}", bytes(in.recvQMax)));
	inRows += rowN("Kernel buffers", nBytes(in.mem));
	inRows += rowN("Drops", kInt(in.drops));
	inRows += rowN("Retransmits", kInt(in.retrans), "on live sockets");
	inRows += rowN("Retransmitting now", kInt(in.retransNow));
	inRows += rowN("Unacked segments", kInt(in.unacked));

	auto pctRow = [](string_view label, const Pct& p, int dec, string_view note) {
		if (!p.ok) {
			return fmt::format("<tr><th>{}</th><td class=\"n\">-</td><td class=\"n\">-</td><td class=\"n\">-</td><td class=\"n\">-</td><td class=\"note\">{}</td></tr>\n", label, note);
		}
		return fmt::format("<tr><th>{}</th>{}{}{}{}<td class=\"note\">{}</td></tr>\n", label, td(nFix(p.p50, dec)), td(nFix(p.p90, dec)),
		                   td(nFix(p.p99, dec)), td(nFix(p.max, dec)), note);
	};
	string lat = headRow({"established inbound", "p50", "p90", "p99", "max", ""});
	lat += pctRow("RTT ms", percentiles(in.rttMs), 2, "");
	lat += pctRow("Idle rx s", percentiles(in.idleS), 1, "since the last data from the peer");

	out += "<div class=\"grid\">\n" + kvSection("", "Inbound", inRows);
	out += "<section><h2>Inbound latency</h2>\n<table class=\"t\">\n" + lat + "</table></section>\n</div>\n";

	std::vector<std::pair<u16, Group>> ports;
	for (const auto& [port, v] : outbound) {
		Group grp;
		for (const auto* s : v) {
			grp.add(*s);
		}
		ports.emplace_back(port, std::move(grp));
	}
	std::sort(ports.begin(), ports.end(), [](const auto& a, const auto& b) { return a.second.n > b.second.n; });
	out += "<section><h2>Outbound by remote port</h2>\n<table class=\"t\">\n";
	out += headRow({"port", "conn", "established", "Send-Q", "Recv-Q", "RTT p50 ms", "RTT p99 ms", "RTT max ms", "retrans", "retransmitting now"});
	for (size_t i = 0; i < ports.size() && i < top; ++i) {
		const auto& [port, grp] = ports[i];
		const auto  p           = percentiles(grp.rttMs);
		auto        rtt         = [&](double v) { return p.ok ? td(nFix(v, 2)) : td("-"); };
		out += fmt::format("<tr>{}{}{}{}{}{}{}{}{}{}</tr>\n", td(fmt::format("{}", port)), td(nInt(grp.n)), td(nInt(grp.est)),
		                   td(nBytes(grp.sendQ)), td(nBytes(grp.recvQ)), rtt(p.p50), rtt(p.p99), rtt(p.max), td(nInt(grp.retrans)),
		                   td(nInt(grp.retransNow)));
	}
	out += "</table></section>\n";

	if (top) {
		// A websocket with a growing Send-Q is a client that stopped reading
		out += topTable("Top Send-Q", conns, top, [](const DiagSock& s) { return s.wq; });
		// Way past the ping interval = a peer that is gone without a FIN
		std::vector<const DiagSock*> estIn;
		for (const auto* s : inbound) {
			if (s->state == TCP_ESTABLISHED) {
				estIn.push_back(s);
			}
		}
		out += topTable("Top idle inbound", estIn, top, [](const DiagSock& s) { return s.lastRecvMs; });
		out += topTable("Top retransmits", conns, top, [](const DiagSock& s) { return s.totalRetrans; });
	}

	const auto t3 = clock::now();
	auto       ms = [](auto a, auto b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
	out += fmt::format("<p class=\"note\">{} sockets in {:.1f} ms: fd walk {:.1f}, sock_diag {:.1f}, render {:.1f}{}</p>\n",
	                   all.size(), ms(t0, t3), ms(t0, t1), ms(t1, t2), ms(t2, t3),
	                   err.empty() ? string() : " · " + warn(escapeH(err)));
	return out;
}

} // namespace statusProbes
