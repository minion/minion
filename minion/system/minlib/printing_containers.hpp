/** \weakgroup MinLib
 * @{
 */

#ifndef TOSTRING_H
#define TOSTRING_H

#include "box.h"

#include "basic_sys.hpp"
#include "optional.hpp"

template <typename T>
void output_container(std::ostream& o, const T& t);

namespace std {
template <typename T, typename U>
std::ostream& operator<<(std::ostream& o, const std::pair<T, U>& p) {
  return o << "(" << p.first << "," << p.second << ")";
}

template <typename T1>
std::ostream& operator<<(std::ostream& o, const TUPLE<T1>& p) {
  return o << "<" << TUPLE_GET<0>(p) << ">";
}

template <typename T1, typename T2>
std::ostream& operator<<(std::ostream& o, const TUPLE<T1, T2>& p) {
  return o << "<" << TUPLE_GET<0>(p) << "," << TUPLE_GET<1>(p) << ">";
}

template <typename T1, typename T2, typename T3>
std::ostream& operator<<(std::ostream& o, const TUPLE<T1, T2, T3>& p) {
  return o << "<" << TUPLE_GET<0>(p) << "," << TUPLE_GET<1>(p) << "," << TUPLE_GET<2>(p) << ">";
}

template <typename T1, typename T2, typename T3, typename T4>
std::ostream& operator<<(std::ostream& o, const TUPLE<T1, T2, T3, T4>& p) {
  return o << "<" << TUPLE_GET<0>(p) << "," << TUPLE_GET<1>(p) << "," << TUPLE_GET<2>(p) << ","
           << TUPLE_GET<3>(p) << ">";
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
std::ostream& operator<<(std::ostream& o, const TUPLE<T1, T2, T3, T4, T5>& p) {
  return o << "<" << TUPLE_GET<0>(p) << "," << TUPLE_GET<1>(p) << "," << TUPLE_GET<2>(p) << ","
           << TUPLE_GET<3>(p) << "," << TUPLE_GET<4>(p) << ">";
}

template <typename T>
std::ostream& operator<<(std::ostream& o, const std::vector<T>& t) {
  output_container(o, t);
  return o;
}

template <typename T>
std::ostream& operator<<(std::ostream& o, const std::list<T>& t) {
  output_container(o, t);
  return o;
}

template <typename T>
std::ostream& operator<<(std::ostream& o, const std::set<T>& t) {
  output_container(o, t);
  return o;
}

template <typename T>
std::ostream& operator<<(std::ostream& o, const std::unordered_set<T>& t) {
  output_container(o, t);
  return o;
}

template <typename T>
std::ostream& operator<<(std::ostream& o, const std::multiset<T>& t) {
  output_container(o, t);
  return o;
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream& o, const std::map<T, U>& t) {
  output_container(o, t);
  return o;
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream& o, const std::unordered_map<T, U>& t) {
  output_container(o, t);
  return o;
}

template <typename T, size_t s>
std::ostream& operator<<(std::ostream& o, const std::array<T, s>& a) {
  output_container(o, a);
  return o;
}

template <typename T>
std::ostream& operator<<(std::ostream& o, const box<T>& a) {
  output_container(o, a);
  return o;
}

template <typename T>
std::ostream& operator<<(std::ostream& o, const option<T>& t) {
  if(t)
    o << *t;
  else
    o << "<empty>";
  return o;
}
}

template <typename T>
void output_container(std::ostream& o, const T& t) {
  o << "[";
  if(!t.empty()) {
    typename T::const_iterator it(t.begin());
    o << *it;
    ++it;
    for(; it != t.end(); ++it) {
      o << ",";
      o << *it;
    }
  }
  o << "]";
}

#endif
/** @}
 */
