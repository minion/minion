#ifndef _ARRAY_FUNCTIONS_HPP_CDSHUICDS
#define _ARRAY_FUNCTIONS_HPP_CDSHUICDS

#include "interval.hpp"
#include <vector>

/** \weakgroup MinLib
 * @{
 */

/** \brief Increments a container of values.
 * \param vals Container to be incremented.
 * \param maxvals Maximum values of each index.
 * \return true if the array was previously at maximum value.
 */
template <typename Container>
bool incrementVector(Container& vals, const Container& maxvals) {
  bool carry = true;
  int position = (int)vals.size() - 1;
  while(position >= 0 && carry == true) {
    D_ASSERT(maxvals[position] > 0);
    vals[position]++;
    if(vals[position] >= maxvals[position])
      vals[position] = 0;
    else
      carry = false;
    --position;
  }
  return !carry;
}

/// I
template <typename T>
std::vector<T> initializeVector_from_intervals(const std::vector<INTERVAL<T>>& intervals) {
  std::vector<T> t;
  for(auto it = intervals.begin(); it != intervals.end(); ++it)
    t.push_back(first(*it));
  return t;
}

/// increments a container of values. Returns 'true' until the maximum value is
/// reached.
template <typename Container, typename IntervalContainer>
bool incrementVector_from_intervals(Container& vals, const IntervalContainer& intervals) {
  bool carry = true;
  int position = (int)vals.size() - 1;
  while(position >= 0 && carry == true) {
    D_ASSERT(!interval_empty(intervals[position]));
    D_ASSERT(contains(intervals[position], vals[position]));
    vals[position]++;
    if(vals[position] > intervals[position].upper())
      vals[position] = intervals[position].lower();
    else
      carry = false;
    --position;
  }
  return !carry;
}

/// Accepts a C<D<T>>, and returns a
/// C<D<T>> which forms the cross product.
/// e.g. { {1,2},{3,4},{5} } maps to
/// { {1,3,5}, {1,4,5}, {2,3,5}, {2,4,5} }
///
/// This function handles correctly both the input array being empty,
/// or any memory of it being empty, in both cases returning an
/// empty container.
template <typename Container>
Container cross_prod(const Container& con) {
  Container outcon;
  if(con.empty())
    return outcon;

  std::vector<int> maxCounter;
  for(auto it = con.begin(); it != con.end(); ++it) {
    if(it->size() == 0)
      return outcon;
    maxCounter.push_back(it->size());
  }

  std::vector<int> sizeCounter(maxCounter.size(), 0);

  do {
    typename Container::value_type inner_con;
    inner_con.reserve(maxCounter.size());
    for(unsigned i = 0; i < maxCounter.size(); ++i)
      inner_con.push_back(con[i][sizeCounter[i]]);
    outcon.push_back(inner_con);
  } while(incrementVector(sizeCounter, maxCounter));

  return outcon;
}

/// Merges a vector<vector<val>> to a vector<val>
template <typename val>
std::vector<val> mergeVectors(const std::vector<std::vector<val>>& v) {
  std::vector<val> ret;
  typedef typename std::vector<std::vector<val>>::const_iterator iter;
  for(iter it = v.begin(); it != v.end(); ++it)
    ret.insert(ret.back(), it->begin(), it->end());
  return ret;
}

/** @}
 */
#endif
