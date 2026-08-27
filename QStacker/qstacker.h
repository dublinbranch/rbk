#ifndef QSTACKER_H
#define QSTACKER_H

#include "CxaLevel.h"
#include <QByteArray>
#include <atomic>
#include <string>

//Used to monitor how many exception are beeing thrown over time by hacheck, a few are ok, too many no!
inline std::atomic<uint> exceptionThrown{0};

struct QStackerOpt {
	bool snippet = true;
	bool object  = true;
	bool address = true;
	// prepend a newline
	bool prependReturn = true;
};
constexpr QStackerOpt QStackerOptLight = {false, false, false};
Q_REQUIRED_RESULT std::string stacker(uint skip = 2, QStackerOpt opt = QStackerOptLight);
Q_REQUIRED_RESULT QByteArray  QStacker(uint skip = 3, QStackerOpt opt = QStackerOptLight);
Q_REQUIRED_RESULT QString     QStacker16(uint skip = 3, QStackerOpt opt = QStackerOptLight);
Q_REQUIRED_RESULT QString     QStacker16Light(uint skip = 4, QStackerOpt opt = QStackerOptLight);

std::string stackerRDX(uint skip = 5);
//This one we use normally as an inline def to avoid include any in non cooperative header file that just suicide the program
void stacker_CERR();
/**
 * @brief StackerMinLevel — app path allow-root for stack traces (e.g. "/root/PPPLC").
 * When set, stacker() keeps contiguous frames under this path from the throw site,
 * skips HTTP-thread noise (beast.cpp `run`), and stops before Boost/Asio/stdlib frames
 * so libdw does not fully resolve the noisy older frames.
 * When empty, every frame is printed; denied modules (libc/libstdc++/Boost/Qt) use
 * dladdr only, libdw runs on app frames. Concurrent stacker() calls skip (empty).
 */
inline std::string StackerMinLevel;
inline uint        stackerMaxFrame = 15;
// Instruction-pointer cache: return address → {function, file, line}.
// Used/clears are updated under stacker's mutex; safe to read anytime.
inline uint              stackerResolveCacheMax    = 65536;
inline std::atomic<uint> stackerResolveCacheUsed   {0};
inline std::atomic<uint> stackerResolveCacheClears {0};
inline std::atomic<uint> stackerResolveCacheHits   {0};
inline std::atomic<uint> stackerResolveCacheMisses {0};
// "used/max clears=N" — for a status page or a one-off log.
std::string stackerResolveCacheInfo();
void        stackerResolveCacheReset();
// Test/A-B: empty StackerMinLevel uses backward Printer::print (new Dwfl every call).
inline bool stackerLegacyFullPrint = false;
// Next throw will not append stack trace, reset after use
inline thread_local bool cxaNoStack = false;

// what level shall we use ? reset after use as critical
inline thread_local CxaLevel cxaLevel = CxaLevel::critical;

void messanger(const QString& msg, CxaLevel level);
#endif // QSTACKER_H
