#pragma once

#include "../mixin/NoCopy.h"
#include <cstddef>

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
	//sometime we do not want to wait until scope exit
	void reset() {
		if (variable) {
			*variable = oldValue;
		}
		variable = nullptr;
	}
	//OF COURSE you must use the returned value! else will be destroyed immediately
	[[nodiscard]] ResetAfterUse(K& key, const K& value) {
		set(key, value);
	}

	//OF COURSE you must use the returned value! else will be destroyed immediately
	[[nodiscard]] ResetAfterUse(K& key, const std::nullptr_t& value) {
		set(key, value);
	}

	~ResetAfterUse() {
		if (variable) {
			*variable = oldValue;
		}
		variable = nullptr;
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
struct OnExit : public NoCopy {
	F func;
	explicit OnExit(F&& f)
	    : func(std::forward<F>(f)) {
	}
	~OnExit() {
		if (doNotCall) {
			return;
		}
		func();
	}
	//do not call any longer
	void reset() {
		doNotCall = true;
	}

      private:
	bool doNotCall = false;
};

//What is that ???
//template <typename F>
//OnExit(F&& frv) -> OnExit<F>;
