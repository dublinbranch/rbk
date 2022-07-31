#ifndef RWGUARD_H
#define RWGUARD_H

#include <atomic>
#include <shared_mutex>

class RWGuard {
      public:
	RWGuard() = default;

	RWGuard(std::shared_mutex* newMutex);
	~RWGuard();

	void setMutex(std::shared_mutex* newMutex);
	void lock();
	void lockExclusive();
	void lockShared();
	void unlock(bool warnOnUnlockError=true);

      private:
	std::shared_mutex* mutex = nullptr;
	//of course this is not if the MUTEX is locked, but if WE locked it (to avoid a try lock unlock... which is orrible)
	std::atomic_bool shared    = false;
	std::atomic_bool exclusive = false;
};

#endif // RWGUARD_H
