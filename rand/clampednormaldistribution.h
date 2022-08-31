#pragma once
#include <random>
class ClampedNormalDistribution {
      public:
	ClampedNormalDistribution() = default;
	ClampedNormalDistribution(double _fixed);
	ClampedNormalDistribution(double _min, double _max, double _mean, double _stddev);
	void setFixed(double _fixed);
	void setParam(double _min, double _max, double _mean, double _stddev);
	void initSeed() const;

	//we can not copy a random number generator. so just recreate
	///It should have just been easier to move around a shared_ptr...
	///but what about using the rng in multithreading ?
	void operator=(const ClampedNormalDistribution& from);
	ClampedNormalDistribution(const ClampedNormalDistribution& from);

	double gen() const;
	~ClampedNormalDistribution();

      private:
	bool primed = false;

	double fixed = 0;
	double min = 0, max = 0, mean = 0, stddev = 0;
	//so this class can be passed around without big problem as const
	mutable bool                       seeded    = false;
	mutable std::mt19937*              generator = nullptr;
	mutable std::normal_distribution<> distribution;
};
