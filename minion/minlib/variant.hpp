#ifndef MINLIB_VARIANT
#define MINLIB_VARIANT

#include <boost/variant/variant.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/get.hpp>

/** \weakgroup MinLib
 * @{
 */

template<typename T>
struct TypeIs_Internal
{
	typedef bool result_type;

	bool operator()(const T&) const { return true; }
	template<typename U>
	bool operator()(const U&) const { return false; }
};

/// Checks if the type in a boost::variant is T
template<typename T, typename U>
bool variant_istype(const U& u)
{ return boost::apply_visitor(TypeIs_Internal<T>(), u); }

/** @}
 */

#endif
