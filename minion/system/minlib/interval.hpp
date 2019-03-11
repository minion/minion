/** \weakgroup MinLib
 * @{
 */

#ifndef INTERVAL_HPP_FDASJKLFDA
#define INTERVAL_HPP_FDASJKLFDA

#include "basic_sys.hpp"

namespace dom {
template <typename T>
class interval {
  T lower_m;
  T upper_m;

public:
  T lower() const {
    return lower_m;
  }
  T upper() const {
    return upper_m;
  }

  interval(T l, T u) : lower_m(l), upper_m(u) {}

  friend std::ostream& operator<<(std::ostream& o, const interval& v) {
    return o << "{" << v.lower_m << ".." << v.upper_m << "}";
  }
};

template <typename T>
bool interval_empty(const interval<T>& t) {
  return t.lower() > t.upper();
}

template <typename T>
bool operator==(const interval<T>& lhs, const interval<T>& rhs) {
  if(interval_empty(lhs)) {
    return interval_empty(rhs);
  } else {
    return lhs.lower() == rhs.lower() && lhs.upper() == rhs.upper();
  }
}

template <typename T>
struct interval_set {
  std::set<T> _s;

  size_t size() const {
    return _s.size();
  }

  friend std::ostream& operator<<(std::ostream& o, const interval_set& v) {
    return o << v._s;
  }
};

template <typename T>
bool operator==(const interval_set<T>& lhs, const interval_set<T>& rhs) {
  return lhs._s == rhs._s;
}
} // namespace dom

#define INTERVAL dom::interval
#define INTERVAL_SET dom::interval_set

template <typename T>
INTERVAL<T> intervalise(T t) {
  return INTERVAL<T>(t, t);
}

template <typename T>
INTERVAL<T> make_interval(T x, T y) {
  return INTERVAL<T>(x, y);
}

namespace dom {
template <typename T>
T first(const INTERVAL_SET<T>& t) {
  return *(t._s.begin());
}

template <typename T>
T last(const INTERVAL_SET<T>& t) {
  auto it = t._s.end();
  --it;
  return *it;
}

template <typename T>
typename std::set<T>::const_iterator elements_begin(const INTERVAL_SET<T>& t) {
  return t._s.begin();
}

template <typename T>
typename std::set<T>::const_iterator elements_end(const INTERVAL_SET<T>& t) {
  return t._s.end();
}

template <typename T>
T first(const INTERVAL<T>& t) {
  return t.lower();
}

template <typename T>
T last(const INTERVAL<T>& t) {
  return t.upper();
}

template <typename T>
bool contains(const INTERVAL<T>& t, const T& val) {
  return t.lower() <= val && val <= t.upper();
}

template <typename T>
bool contains(const INTERVAL_SET<T>& t, const T& val) {
  return t._s.count(val);
}

template <typename T>
bool contains(const INTERVAL_SET<T>& t, const INTERVAL<T>& i) {
  for(T it = first(i); it <= last(i); ++it)
    if(!contains(t, it))
      return false;
  return true;
}
} // namespace dom

template <typename T>
void is_insert(INTERVAL_SET<T>& is, INTERVAL<T> in) {
  for(T i = first(in); i <= last(in); ++i)
    is._s.insert(i);
}

template <typename T>
void is_insert(INTERVAL_SET<T>& is, T i) {
  is._s.insert(i);
}

template <typename T>
bool single_range(const INTERVAL_SET<T>& t) {
  return last(t) - first(t) + 1 == static_cast<T>(t.size());
}

template <typename T>
INTERVAL<T> intervalise(INTERVAL<T> t) {
  return t;
}

template <typename T>
void push_back_interval_set(T&) {}

template <typename T, typename Arg, typename... Args>
void push_back_interval_set(T& t, Arg a, Args... args) {
  is_insert(t, a);
  push_back_interval_set(t, args...);
}

template <typename Domain, typename... Args>
INTERVAL_SET<Domain> make_interval_set(Args... args) {
  INTERVAL_SET<Domain> is;
  push_back_interval_set(is, intervalise(args)...);
  return is;
}

template <typename Domain>
INTERVAL_SET<Domain> interval_set_from_bounds(Domain x, Domain y) {
  return make_interval_set<Domain>(make_interval<Domain>(x, y));
}

template <typename T, typename... Args>
std::vector<INTERVAL<T>> intervalise_list(Args... args) {
  std::vector<INTERVAL<T>> t;
  push_back(t, intervalise(args)...);
  return t;
}

template <typename T>
struct is_interval {
  enum { val = 0 };
};

template <typename T>
struct is_interval<INTERVAL<T>> {
  enum { val = 1 };
};

template <typename T>
INTERVAL<T> clamp_interval(INTERVAL<T> t, T min_val, T max_val) {
  if(first(t) == std::numeric_limits<T>::min())
    t = make_interval(min_val, last(t));
  if(last(t) == std::numeric_limits<T>::max())
    t = make_interval(first(t), max_val);
  return t;
}

#endif

/** @}
 */
