#ifndef QELAPSEDTIMERV2_H
#define QELAPSEDTIMERV2_H

#include <QElapsedTimer>
#include <string>

class QElapsedTimerV2 : public QElapsedTimer {
      public:
	// Function to format a number with custom thousand and decimal separators
	[[nodiscard]] std::string        format();
	[[nodiscard]] static std::string format(qint64 elapsed_time);
};

#endif // QELAPSEDTIMERV2_H
