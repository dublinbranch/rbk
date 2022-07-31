#ifndef VALUEMITWARNING_H
#define VALUEMITWARNING_H

//This is basically hardcoded to be used inside ONLY swaptronic
//USE mapV2, and figure out something better ?

#include "funkz.h"
#include <QDebug>
#include <QStacker/qstacker.h>

template <typename MType, typename Key>
auto valueMitWarn(const MType& map, const Key& key) {
	using Value = typename MType::mapped_type;
	Value v;
	auto  iter = map.constFind(key);
	if (iter == map.cend()) {
		if (runnable("valueMitWarn", 60)) {
			qWarning().noquote() << key << "not found, this should not happen!" << QStacker16();
		}
	} else {
		v = iter.value();
	}
	return v;
}

#endif // VALUEMITWARNING_H
