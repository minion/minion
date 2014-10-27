#ifndef MAPPER_HPP_CD
#define MAPPER_HPP_CD

#include "variadic.hpp"
#include <vector>
#include <set>
#include "multdim_container.hpp"

template<typename Map, typename V>
std::vector<typename ObjectResult<Map, V>::type >
doMap(Map m, const std::vector<V>& v)
{
	std::vector<typename ObjectResult<Map, V>::type> vec;
	vec.reserve(v.size());
	for(auto it = v.begin(); it != v.end(); ++it)
		vec.push_back(m(*it));
	return vec;
}

template<typename Map, typename V>
std::set<typename ObjectResult<Map, V>::type >
doMap(Map m, const std::set<V>& v)
{
	std::set<typename ObjectResult<Map, V>::type > vec;
	for(auto it = v.begin(); it != v.end(); ++it)
		vec.insert(m(*it));
	return vec;
}


template<typename Map, typename Index, typename Result>
MultiDimCon<Index, typename ObjectResult<Map, Result>::type>
doMap(Map m, const MultiDimCon<Index, Result>& mdc)
{
	MultiDimCon<Index, typename ObjectResult<Map, Result>::type> res_mdc(mdc.getBounds());
	for(auto it = mdc.indices.begin(); it != mdc.indices.end(); ++it)
	{
		res_mdc.indices.insert(make_pair(it->first, m(it->second)));
	}

	return res_mdc;
}


#endif