#include "rbk/QStacker/qstacker.h"
#include "backward.hpp"
#include "rbk/QStacker/exceptionv2.h"
#include "rbk/string/util.h"
#include <QDebug>
#include <QString>
#include <boost/algorithm/string.hpp>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <execinfo.h>
#endif

std::string stacker(uint skip, QStackerOpt opt) {
	/** For loading from an arbitrary position
	ucontext_t uctx;
	getcontext(&uctx);
	void* error_addr = reinterpret_cast<void*>(uctx.uc_mcontext.gregs[REG_RIP]);
	st.load_from(error_addr, 32);
	*/
	using namespace backward;
	static std::mutex            mu;
	std::unique_lock<std::mutex> lock(mu, std::try_to_lock);
	if (!lock.owns_lock()) {
		//too much concurret request to stacker ? something bad is happening!!!
		return "stacker already in use!";
	}

	StackTrace st;
	st.load_here(stackerMaxFrame);
	st.skip_n_firsts(skip); //skip internal lib stuff

	Printer p;
	p.snippet = opt.snippet;
	p.object  = opt.object;
	p.address = opt.address;

	std::ostringstream stream;
	p.print(st, stream);
	std::string str = "\n" + stream.str();

	//Remove all the stuff before our process (if set)
	if (!StackerMinLevel.empty()) {
		//After we move to Qt 6 we will use QByteArrayView which is just better

		auto start = str.find(StackerMinLevel);
		if (start == std::string::npos) {
			return str;
		}
		//we now have to find the line
		auto end = str.find('\n', start);
		//start is 1 char after the previous \n
		start = str.rfind('\n', start);

		auto row = subView(str, start, end);
		//to remove the asio noise
		if (row.contains("rbk/HTTP/beast.cpp:") && row.contains("operator()")) {
			start = str.find(StackerMinLevel, end);
			if (start == std::string::npos) { //in case of expection INSIDE the asio / beast stack
				if (opt.prependReturn) {
					str = str.substr(start);
				} else {
					str = str.substr(start + 1);
				}
			}
		}

		start = str.rfind('\n', start);
		if (opt.prependReturn) {
			str = str.substr(start);
		} else {
			str = str.substr(start + 1);
		}
	}
	return str;
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
		static const QString x;
		static const auto    qstringCode          = typeid(x).hash_code();
		static const auto    stdExceptionTypeCode = typeid(std::exception).hash_code();

		auto exceptionTypeCode = ((std::type_info*)pvtinfo)->hash_code();

		QString msg;

		if (cxaNoStack) {
			cxaNoStack = false;
		} else {
			msg = QStacker16Light(5);
		}

		if (exceptionTypeCode == qstringCode) { //IF QString has been thrown is by us, and usually handled too
			auto th = static_cast<QString*>(thrown_exception);
			msg.prepend(*th);
		} else if (exceptionTypeCode == stdExceptionTypeCode) {
			auto th = static_cast<std::exception*>(thrown_exception);
			msg.prepend(th->what());
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

#if WIN32
backward::SignalHandling sh;
#endif

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
