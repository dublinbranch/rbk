#ifndef SQLROWV2SWAP_H
#define SQLROWV2SWAP_H

#include <boost/describe.hpp>
#include <boost/describe/class.hpp>
#include <boost/mp11/algorithm.hpp>

#include "rbk/minMysql/sqlrowv2.h"
#include "rbk/types/isDescribed.h"
#include "rbk/types/isOptional.h"

#include "rbk/minMysql/sqlcomposer.h"

//To use this the mapped obj need to be described

template <class T,
          class Bd = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
          class Md = boost::describe::describe_members<T, boost::describe::mod_any_access>>
        requires isDescribedClass<T>
void rowSwap(const SqlRowV2& row, T& dest) {
	boost::mp11::mp_for_each<Bd>([&](auto D) {
		using B = typename decltype(D)::type;
		rowSwap((B const&)row, dest);
	});

	boost::mp11::mp_for_each<Md>([&](auto D) {
		using Type = std::remove_cvref_t<decltype(dest.*D.pointer)>;

		if constexpr (boost::describe::has_describe_members<Type>::value) {
			// Nested struct, call recursiveTraversal
			rowSwap<Type>(row, dest.*D.pointer);
		} else {
			// Simple field, process it
			if constexpr (is_optional<Type>) {
				row.get(D.name, dest.*D.pointer);
			} else {
				row.rq(D.name, dest.*D.pointer);
			}
		}
	});
}

//Always optional
template <class T,
          class Bd = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
          class Md = boost::describe::describe_members<T, boost::describe::mod_any_access>>
        requires isDescribedClass<T>
void rowSwapGet(const SqlRowV2& row, T& dest) {
	boost::mp11::mp_for_each<Bd>([&](auto D) {
		using B = typename decltype(D)::type;
		rowSwap((B const&)row, dest);
	});

	boost::mp11::mp_for_each<Md>([&](auto D) {
		using Type = std::remove_cvref_t<decltype(dest.*D.pointer)>;

		if constexpr (boost::describe::has_describe_members<Type>::value) {
			// Nested struct, call recursiveTraversal
			rowSwapGet<Type>(row, dest.*D.pointer);
		} else {
			// Simple field, process it
			row.get(D.name, dest.*D.pointer);
		}
	});
}

template <class T,
          class Bd = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
          class Md = boost::describe::describe_members<T, boost::describe::mod_any_access>>
        requires isDescribedClass<T>
std::vector<T> resultSwap(const SqlResultV2& res) {
	std::vector<T> result;
	for (auto& row : res) {
		T t;
		rowSwap(row, t);
		result.push_back(std::move(t));
	}
	return result;
}

template <class T,
          class Bd = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
          class Md = boost::describe::describe_members<T, boost::describe::mod_any_access>>
        requires isDescribedClass<T>
void toSql(const T& src, SqlComposer& sql) {
	boost::mp11::mp_for_each<Bd>([&](auto D) {
		using B = typename decltype(D)::type;
		toSql((B const&)src, sql);
	});

	boost::mp11::mp_for_each<Md>([&](auto D) {
		using Type = std::remove_cvref_t<decltype(src.*D.pointer)>;

		if constexpr (boost::describe::has_describe_members<Type>::value) {
			// Nested struct, call recursiveTraversal
			toSql(src, sql);
		} else {
			//TODO handle optional to emulate null ?
			sql.push(D.name, src.*D.pointer);
		}
	});
}

#endif // SQLROWV2SWAP_H
