#ifndef TIMESPECV2_H
#define TIMESPECV2_H

#include <compare>
#include <sys/types.h>
#include "rbk/number/intTypes.h"

struct timespec;
class TimespecV2 {
      public:
	//This can easily handle microsecond precision data 53bit / 15.95digit, a unix ts is 10 digit
	double time;

	int sec() const;

	//this will return 0.12345 clipping out the integer part
	double fractional() const;

	//no idea why anyone will use those but let's leave them
	uint ms() const;
	uint ns() const;

	auto operator<=>(const TimespecV2& t) const = default;

	TimespecV2() = default;
	TimespecV2(double ts);
	TimespecV2        operator-(const TimespecV2& rhs) const;
	TimespecV2        operator-(const double& rhs) const;
	timespec          toTimespec() const;
	static TimespecV2 now();
	void              setNow();
	double            toDouble() const;

	operator double() const;
};

#endif // TIMESPECV2_H
