#ifndef QELAPSEDTIMERV2_H
#define QELAPSEDTIMERV2_H

#include <QElapsedTimer>
#include <string>

class QElapsedTimerV2 : public QElapsedTimer {
      public:
	// Function to format a number with custom thousand and decimal separators
	std::string format();
};

#endif // QELAPSEDTIMERV2_H
