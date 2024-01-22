#ifndef ROWSWAP_H
#define ROWSWAP_H

#include "rbk/minMysql/sqlRow.h"
#include "rbk/minMysql/sqlresult.h"
#include "rowSwappable.h"
#include <boost/describe.hpp>
#include <boost/describe/class.hpp>
#include <boost/mp11/algorithm.hpp>

template <class T,
          class Bd = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
          class Md = boost::describe::describe_members<T, boost::describe::mod_any_access>>
void rowSwap(const sqlRow& row, T& dest) {
	boost::mp11::mp_for_each<Bd>([&](auto D) {
		using B = typename decltype(D)::type;
		rowSwap((B const&)row, dest);
	});
	boost::mp11::mp_for_each<Md>([&](auto D) {
		using Type = std::remove_cvref_t<decltype(dest.*D.pointer)>;
		if constexpr (isRowSwappableType<Type>) {
			// Nested struct, call recursiveTraversal
			rowSwap<Type>(row, dest.*D.pointer);
		} else {
			// Simple field, process it
			row.get2(D.name, dest.*D.pointer);
		}
	});
}

template <class T,
          class Bd = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
          class Md = boost::describe::describe_members<T, boost::describe::mod_any_access>>
void rowSwapRq(const sqlRow& row, T& dest) {
	boost::mp11::mp_for_each<Bd>([&](auto D) {
		using B = typename decltype(D)::type;
		rowSwapRq((B const&)row, dest);
	});
	boost::mp11::mp_for_each<Md>([&](auto D) {
		using Type = std::remove_cvref_t<decltype(dest.*D.pointer)>;
		if constexpr (isRowSwappableType<Type>) {
			// Nested struct, call recursiveTraversal
			rowSwapRq<Type>(row, dest.*D.pointer);
		} else {
			// Simple field, process it
			row.rq(D.name, dest.*D.pointer);
		}
	});
}

#endif // ROWSWAP_H
