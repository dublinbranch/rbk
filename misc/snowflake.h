#ifndef SNOWFLAKE_H
#define SNOWFLAKE_H

//This is named after https://en.wikipedia.org/wiki/Snowflake_ID 

#include "rbk/dateTime/timespecV2.h"
#include "rbk/mixin/NoCopy.h"
#include <atomic>
class Snowflake : public NoCopy {
	using u64 = uint64_t;

      public:
	Snowflake(u64 _srvId) {
		this->srvId = _srvId;
	}

	u64 next();

	u64  toUint() const;
	void fromUint(const u64 v);

	bool operator==(const Snowflake& j) const;

      private:
	struct Adp {
		u64 mask = 0;
		u64 pad  = 0;
	};

    inline static constinit Adp srvIdX = {(((1ull << 8) - 1) << 10), 10};
	inline static constinit Adp seqX   = {1023, 0};
    inline static constinit Adp msX    = {(((1ull << 10) - 1) << 18), 18};
    inline static constinit Adp tsX    = {(((1ull << 32) - 1) << 28), 28};

	//In theory we should check if we exceed 1024 values in a millisecond, I hardly believe this will happen!
	std::atomic<u64> seq   = 0; //10
	u64              srvId = 0; //8 + 10pad
	std::atomic<u64> ms    = 0; //10 + 18pad
	std::atomic<u64> ts    = 0; //32 bit + 28pad = 60
	                            //4 bit are reserved atm

	//Each time the time changes we will reset the seq
	std::atomic<TimespecV2> lastUse;
	void                    testMe();
};

#endif // SNOWFLAKE_H
