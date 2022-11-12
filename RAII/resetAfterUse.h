#pragma once

/**
 *small helper class used to set and reset to initial value call like
 *ResetAfterUse r{rapidAssertEnabled, false};
 */
template <typename K>
class ResetAfterUse {
      public:
	ResetAfterUse() = default;
	void set(K& key, const K& value) {
		//Copy the value
		oldValue = key;
		//Assign the new one
		key = value;
		//Keep a reference
		this->variable = &key;
	}
	//OF COURSE you must use the returned value! else will be destroyed immediately
	[[nodiscard]] ResetAfterUse(K& key, const K& value) {
		set(key, value);
	}
	~ResetAfterUse() {
		*variable = oldValue;
	}

      private:
	K* variable = nullptr;
	K  oldValue;
};

template <typename K>
class SetOnExit {
      public:
	SetOnExit() = default;

	void set(K& key, const K& value) {
		//Copy the value
		nextValue = value;

		//Keep a reference
		this->variable = &key;
	}
	SetOnExit(K& key, const K& value) {
		set(key, value);
	}
	~SetOnExit() {
		*variable = nextValue;
	}

      private:
	K* variable = nullptr;
	K  nextValue;
};

#include <utility>
template <typename F>
struct OnExit {
	F func;
	OnExit(F&& f)
	    : func(std::forward<F>(f)) {
	}
	~OnExit() {
		func();
	}
};

template <typename F>
OnExit(F&& frv) -> OnExit<F>;
