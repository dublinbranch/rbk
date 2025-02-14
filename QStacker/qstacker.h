#ifndef QSTACKER_H
#define QSTACKER_H

#include "CxaLevel.h"
#include <QByteArray>

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
Q_REQUIRED_RESULT std::string stacker(uint skip = 3, QStackerOpt opt = QStackerOptLight);
Q_REQUIRED_RESULT QByteArray  QStacker(uint skip = 4, QStackerOpt opt = QStackerOptLight);
Q_REQUIRED_RESULT QString     QStacker16(uint skip = 4, QStackerOpt opt = QStackerOptLight);
Q_REQUIRED_RESULT QString     QStacker16Light(uint skip = 5, QStackerOpt opt = QStackerOptLight);

std::string stackerRDX(uint skip = 5);
//This one we use normally as an inline def to avoid include any in non cooperative header file that just suicide the program
void stacker_CERR();
/**
 * @brief StackerMinLevel is used to cut away the initial part of the stack trace
 * execially when running a thread there is a lot of stuff
 * It must be the name of the program, or the folder from which the code has been compiled
 */
inline std::string StackerMinLevel;
inline uint        stackerMaxFrame = 15;
// Next throw will not append stack trace, reset after use
inline thread_local bool cxaNoStack = false;

// what level shall we use ? reset after use as critical
inline thread_local CxaLevel cxaLevel = CxaLevel::critical;

void messanger(const QString& msg, CxaLevel level);
#endif // QSTACKER_H
