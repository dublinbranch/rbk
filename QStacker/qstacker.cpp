#include "rbk/QStacker/qstacker.h"
#include "backward.hpp"
#include "rbk/QStacker/exceptionv2.h"
#include <QDebug>
#include <QString>
#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdlib>
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#endif

namespace {

constexpr size_t kMaxPrintedName = 120;
constexpr size_t kMaxMangled     = 160;

bool contains(std::string_view hay, std::string_view needle) {
	return hay.find(needle) != std::string_view::npos;
}

bool pathIsDenied(std::string_view path) {
	if (path.empty()) {
		return false;
	}
	return contains(path, "/boost/") || contains(path, "/bits/") || contains(path, "x-tools") ||
	       contains(path, "/sysroot/") || contains(path, "libc.so") || contains(path, "libstdc++") ||
	       contains(path, "libgcc_s") || contains(path, "ld-linux") ||
	       contains(path, "crosstool-ng") || contains(path, "/usr/include/c++/") ||
	       contains(path, "libQt") || contains(path, "/opt/Qt/");
}

bool symbolIsDenied(std::string_view sym) {
	if (sym.empty()) {
		return false;
	}
	// Header-only Boost/Asio lives in the main binary; dladdr module path is not /boost/.
	return contains(sym, "boost::") || contains(sym, "asio::") ||
	       contains(sym, "_ZN5boost") || contains(sym, "4asio");
}

bool pathIsApp(std::string_view path) {
	if (path.empty() || StackerMinLevel.empty()) {
		return false;
	}
	if (pathIsDenied(path)) {
		return false;
	}
	return contains(path, StackerMinLevel);
}

bool functionIsRun(std::string_view fn) {
	return fn == "run" || fn.ends_with("::run");
}

bool isBeastRunNoise(const backward::ResolvedTrace& t) {
	auto check = [](std::string_view file, std::string_view fn) {
		return contains(file, "rbk/HTTP/beast.cpp") && functionIsRun(fn);
	};
	if (check(t.source.filename, t.source.function)) {
		return true;
	}
	for (const auto& in : t.inliners) {
		if (check(in.filename, in.function)) {
			return true;
		}
	}
	return false;
}

bool isAppFrame(const backward::ResolvedTrace& t) {
	if (pathIsApp(t.source.filename)) {
		return true;
	}
	for (const auto& in : t.inliners) {
		if (pathIsApp(in.filename)) {
			return true;
		}
	}
	// No usable source path: drop known libs; do not keep ambiguous frames when filtering.
	if (t.source.filename.empty()) {
		return false;
	}
	return false;
}

void capTraceNames(backward::ResolvedTrace& t) {
	auto cap = [](std::string& s) {
		if (s.size() > kMaxPrintedName) {
			s.resize(kMaxPrintedName);
			s += "...";
		}
	};
	cap(t.object_function);
	cap(t.source.function);
	for (auto& in : t.inliners) {
		cap(in.function);
	}
}

#ifndef _WIN32
std::string cheapDemangle(const char* name) {
	if (!name || !*name) {
		return {};
	}
	const auto len = std::strlen(name);
	if (len > kMaxMangled) {
		return std::string(name, name + 64) + "...";
	}
	int   status = 0;
	char* dem    = abi::__cxa_demangle(name, nullptr, nullptr, &status);
	if (!dem) {
		return name;
	}
	std::unique_ptr<char, decltype(&std::free)> guard(dem, &std::free);
	return std::string(dem);
}

backward::ResolvedTrace resolveFromDladdr(const backward::Trace& tr, const Dl_info& dli) {
	backward::ResolvedTrace t(tr);
	if (dli.dli_fname) {
		t.object_filename = dli.dli_fname;
	}
	t.object_function = cheapDemangle(dli.dli_sname);
	capTraceNames(t);
	return t;
}

bool dladdrIsDenied(const Dl_info& dli) {
	if (dli.dli_fname && pathIsDenied(dli.dli_fname)) {
		return true;
	}
	if (dli.dli_sname && symbolIsDenied(dli.dli_sname)) {
		return true;
	}
	return false;
}
#endif

// Reused under stacker's try_lock mutex: one Dwfl session + IP → {fn,file,line} cache.
struct StackerResolve {
	backward::TraceResolver                                    resolver;
	std::unordered_map<void*, backward::ResolvedTrace>         cache;

	void remember(void* addr, const backward::ResolvedTrace& t) {
		if (!addr || stackerResolveCacheMax == 0) {
			return;
		}
		if (cache.size() >= stackerResolveCacheMax) {
			cache.clear();
			stackerResolveCacheClears.fetch_add(1, std::memory_order_relaxed);
		}
		cache.emplace(addr, t);
		stackerResolveCacheUsed.store(static_cast<uint>(cache.size()), std::memory_order_relaxed);
	}

	backward::ResolvedTrace resolve(const backward::Trace& tr) {
		if (tr.addr) {
			if (auto it = cache.find(tr.addr); it != cache.end()) {
				stackerResolveCacheHits.fetch_add(1, std::memory_order_relaxed);
				auto t = it->second;
				t.idx  = tr.idx;
				return t;
			}
		}

#ifndef _WIN32
		Dl_info dli{};
		if (tr.addr && dladdr(tr.addr, &dli) && dladdrIsDenied(dli)) {
			stackerResolveCacheMisses.fetch_add(1, std::memory_order_relaxed);
			auto t = resolveFromDladdr(tr, dli);
			remember(tr.addr, t);
			return t;
		}
#endif
		stackerResolveCacheMisses.fetch_add(1, std::memory_order_relaxed);
		auto t = resolver.resolve(tr);
		capTraceNames(t);
		remember(tr.addr, t);
		return t;
	}
};

void printResolved(backward::Printer& p, const std::vector<backward::ResolvedTrace>& frames,
                   std::ostream& stream, size_t threadId) {
	// Printer iterator path does not reverse; emit oldest-first (most recent last).
	p.print(frames.rbegin(), frames.rend(), stream, threadId);
}

std::mutex     stackerMu;
StackerResolve stackerState;

} // namespace

void stackerResolveCacheReset() {
	// Blocking: this is explicit (tests / diagnostics), not the hot dump path.
	std::lock_guard<std::mutex> lock(stackerMu);
	stackerState.cache.clear();
	stackerResolveCacheUsed.store(0, std::memory_order_relaxed);
	stackerResolveCacheClears.store(0, std::memory_order_relaxed);
	stackerResolveCacheHits.store(0, std::memory_order_relaxed);
	stackerResolveCacheMisses.store(0, std::memory_order_relaxed);
}

std::string stacker(uint skip, QStackerOpt opt) {
	/** For loading from an arbitrary position
	ucontext_t uctx;
	getcontext(&uctx);
	void* error_addr = reinterpret_cast<void*>(uctx.uc_mcontext.gregs[REG_RIP]);
	st.load_from(error_addr, 32);
	*/
	using namespace backward;
	std::unique_lock<std::mutex> lock(stackerMu, std::try_to_lock);
	if (!lock.owns_lock()) {
		// Concurrent stacker: something is already very wrong; do not add more spam.
		return {};
	}

	StackTrace st;
	st.load_here(stackerMaxFrame);
	st.skip_n_firsts(skip); //skip internal lib stuff

	Printer p;
	p.snippet = opt.snippet;
	p.object  = opt.object;
	p.address = opt.address;

	std::ostringstream stream;

	if (stackerLegacyFullPrint) {
		p.print(st, stream);
		std::string str = stream.str();
		if (opt.prependReturn) {
			str = "\n" + str;
		}
		return str;
	}

	std::vector<ResolvedTrace> frames;
	frames.reserve(st.size());

	if (StackerMinLevel.empty()) {
		// Full backtrace: every frame, but libdw only for non-denied modules.
		for (size_t i = 0; i < st.size(); ++i) {
			auto t = stackerState.resolve(st[i]);
			t.idx  = i;
			frames.push_back(std::move(t));
		}
		printResolved(p, frames, stream, st.thread_id());
	} else {
		// Contiguous app frames only; stop before Boost/Asio/stdlib.
		for (size_t i = 0; i < st.size(); ++i) {
			auto t = stackerState.resolve(st[i]);

			if (pathIsDenied(t.object_filename) || pathIsDenied(t.source.filename) ||
			    symbolIsDenied(t.object_function)) {
				if (!frames.empty()) {
					break;
				}
				continue;
			}
			if (isBeastRunNoise(t)) {
				break;
			}
			if (isAppFrame(t)) {
				t.idx = frames.size();
				frames.push_back(std::move(t));
			} else if (!frames.empty()) {
				break;
			}
		}

		if (frames.empty()) {
			// Fallback: nothing matched the allow-root (e.g. stripped build) — hybrid full dump.
			for (size_t i = 0; i < st.size(); ++i) {
				auto t = stackerState.resolve(st[i]);
				t.idx  = i;
				frames.push_back(std::move(t));
			}
		}
		printResolved(p, frames, stream, st.thread_id());
	}

	std::string str = stream.str();
	if (opt.prependReturn) {
		str = "\n" + str;
	}
	return str;
}

std::string stackerResolveCacheInfo() {
	return std::to_string(stackerResolveCacheUsed.load()) + "/" +
	       std::to_string(stackerResolveCacheMax) + " clears=" +
	       std::to_string(stackerResolveCacheClears.load()) + " hits=" +
	       std::to_string(stackerResolveCacheHits.load()) + " misses=" +
	       std::to_string(stackerResolveCacheMisses.load());
}

QByteArray QStacker(uint skip, QStackerOpt opt) {
	return QByteArray::fromStdString(stacker(skip, opt));
}
QString QStacker16(uint skip, QStackerOpt opt) {
	return QString::fromStdString(stacker(skip, opt));
}

///** ***************/
///** POWER SUPREME */
///** ***************/

//define the functor
using cxa_throw_type = void(void*, std::type_info*, void (*)(void*));
//now take the address of the REAL __cxa_throw
//static cxa_throw_type* original_cxa_throw = (cxa_throw_type*)dlsym(RTLD_NEXT, "__cxa_throw");

// Cross-platform symbol resolution
static cxa_throw_type* resolve_cxa_throw() {
#ifdef _WIN32
	// Windows equivalent for dynamic symbol lookup
	HMODULE handle = GetModuleHandle(nullptr); // Get the current module
	if (!handle) {
		return nullptr;
	}
	FARPROC func = GetProcAddress(handle, "__cxa_throw");
	if (!func) {
		return nullptr;
	}
	//windows is still in 16bit era with FARPROC
// Suppress the warning for this specific cast
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
	auto result = reinterpret_cast<cxa_throw_type*>(func);
#pragma GCC diagnostic pop
	return result;
#else
	// POSIX dynamic symbol lookup
	return reinterpret_cast<cxa_throw_type*>(dlsym(RTLD_NEXT, "__cxa_throw"));
#endif
}

static cxa_throw_type* original_cxa_throw = resolve_cxa_throw();

//Looks like on windows this trick is not ok, we have to check how bombela does that!

#ifdef __linux__

extern "C" {
//And NOW override it

/// If we use the full signature it will complain we are redefining something
//void __cxa_throw(void*           thrown_exception,
//                 std::type_info* pvtinfo,
//                 void (*dest)(void*)) {

#if defined(__clang__)
void __attribute__((__noreturn__)) __cxa_throw(
    void*           thrown_exception,
    std::type_info* pvtinfo,
    void (*dest)(void*)) {
#else
void __attribute__((__noreturn__)) __cxa_throw(
    void* thrown_exception,
    void* pvtinfo,
    void (*dest)(void*)) {
#endif

	exceptionThrown++;

	//New (as of 12/2020 way of managing excetion, with ExceptionV2
	//force a cast and look for our token
	if (ExceptionV2::isExceptionV2Derived(thrown_exception)) {
		const auto* v2 = static_cast<ExceptionV2*>(thrown_exception);
		if (v2->forcePrint) {
			qCritical() << v2->what();
		}
		/* Our exception ALWAYS carry the trowing point
		 * The exception point will be printed in case of missed catch
		 * In fact we have to do nothing to properly managed them!
		 */
		original_cxa_throw(thrown_exception, (std::type_info*)pvtinfo, dest);
	}
	if (cxaLevel != CxaLevel::none) {
		//Normal exception are soft retarded -.- as you simply do not know where they started!

		QString msg;

		if (cxaNoStack) {
			cxaNoStack = false;
		} else {
			msg = QStacker16Light(5);
		}

		switch (cxaLevel) {
		case CxaLevel::warn:
			qWarning().noquote() << msg;
			break;
		case CxaLevel::debug:
			qDebug().noquote() << msg;
			break;
		case CxaLevel::critical:
			qWarning().noquote() << msg;
			break;
		case CxaLevel::none:
			//none mostly to avoid the warning
			break;
		}
	}

	//reset after use
	cxaLevel = CxaLevel::critical;
	//this will pass tru the exception to the original handler so the program will not catch fire after an exception is thrown
	original_cxa_throw(thrown_exception, (std::type_info*)pvtinfo, dest);

	//we should never reach this point, but the compiler do not recognize the original_cxa_throw above so we put another one here
	throw std::runtime_error("This should never happen!");
}
}

#endif

// Backward handles fatal signals. SIGSEGV is left to libasan when ASan is active;
// otherwise backward keeps SIGSEGV so you still get a stack trace without ASan.
namespace {

bool asanHandlesSegv() {
/**
 * We also have the compile time flag in case the macro defined(__SANITIZE_ADDRESS__) is not working or other weird edge case
Just add this  in the pri or cmake file

	DEFINES += WITH_ASAN

 * 
 */


#if defined(WITH_ASAN) || defined(__SANITIZE_ADDRESS__)
	return true;
#elif defined(_WIN32)
	return false;
#else
	void* sym = dlsym(RTLD_DEFAULT, "__asan_init");
	return sym != nullptr;
#endif
}

std::vector<int> backwardFatalSignals() {
	auto sigs = backward::SignalHandling::make_default_signals();
	if (asanHandlesSegv()) {
		sigs.erase(std::remove(sigs.begin(), sigs.end(), SIGSEGV), sigs.end());
	}
	return sigs;
}

} // namespace

backward::SignalHandling sh(backwardFatalSignals());

QString QStacker16Light(uint skip, QStackerOpt opt) {
	return QStacker16(skip, opt);
}

void messanger(const QString& msg, CxaLevel level) {
	switch (level) {
	case CxaLevel::critical:
		qCritical() << msg;
		return;
	case CxaLevel::debug:
		qDebug() << msg;
		return;
	case CxaLevel::warn:
		qWarning() << msg;
		return;
	case CxaLevel::none:
		return;
	}
}

std::string stackerRDX(uint skip) {
	return stacker(skip);
}

void stacker_CERR() {
	std::cerr << stacker(6);
}
