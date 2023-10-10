// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef _SYSTEM_WRAPPER_H
#define _SYSTEM_WRAPPER_H

#include <ostream>

template <typename T>
struct Wrapper {
  T t;

  Wrapper(const T& _t) : t(_t) {}

  Wrapper() : t() {}

  static void overflow_check(long double d) {
    if(d >= (long double)std::numeric_limits<T>::max() / 1.2 ||
       d <= (long double)std::numeric_limits<T>::min() / 1.2)
      throw std::runtime_error("Numeric over/underflow");
  }
  Wrapper& operator+=(const Wrapper& w) {
    overflow_check((long double)t + (long double)w.t);
    t += w.t;
    return *this;
  }

  Wrapper& operator*=(const Wrapper& w) {
    overflow_check((long double)t * (long double)w.t);
    t *= w.t;
    return *this;
  }

  Wrapper& operator-=(const Wrapper& w) {
    overflow_check((long double)t - (long double)w.t);
    t -= w.t;
    return *this;
  }

  Wrapper& operator/=(const Wrapper& w) {
    overflow_check((long double)t / (long double)w.t);
    t /= w.t;
    return *this;
  }

  Wrapper operator-() const {
    return -t;
  }

  void operator++() {
    overflow_check((long double)(t + 1));
    t++;
  }

  void operator--() {
    overflow_check((long double)(t + 1));
    t--;
  }

  Wrapper operator++(int) {
    Wrapper t(*this);
    ++(*this);
    return t;
  }

  Wrapper operator--(int) {
    Wrapper t(*this);
    --(*this);
    return t;
  }

  friend std::ostream& operator<<(std::ostream& o, Wrapper v) {
    return o << v.t;
  }

  friend std::ostream& operator>>(std::ostream& o, Wrapper v) {
    return o >> v.t;
  }
};

#ifdef WRAP_BOOL_OPS
#undef WRAP_BOOL_OPS
#endif

#define WRAP_BOOL_OPS(op)                                                                          \
  template <typename T>                                                                            \
  bool operator op(const Wrapper<T>& t1, const Wrapper<T>& t2) {                                   \
    return t1.t op t2.t;                                                                           \
  }                                                                                                \
                                                                                                   \
  template <typename T, typename U>                                                                \
  bool operator op(const U& t1, const Wrapper<T>& t2) {                                            \
    return t1 op t2.t;                                                                             \
  }                                                                                                \
                                                                                                   \
  template <typename T, typename U>                                                                \
  bool operator op(const Wrapper<T>& t1, const U& t2) {                                            \
    return t1.t op t2;                                                                             \
  }

WRAP_BOOL_OPS(==)
WRAP_BOOL_OPS(!=)
WRAP_BOOL_OPS(<)
WRAP_BOOL_OPS(>)
WRAP_BOOL_OPS(<=)
WRAP_BOOL_OPS(>=)

#ifdef WRAP_ARITHMETIC_OPS
#undef WRAP_ARITHMETIC_OPS
#endif

#define WRAP_ARITHMETIC_OPS(op)                                                                    \
  template <typename T>                                                                            \
  Wrapper<T> operator op(const Wrapper<T>& t1, const Wrapper<T>& t2) {                             \
    return t1.t op t2.t;                                                                           \
  }                                                                                                \
                                                                                                   \
  template <typename T, typename U>                                                                \
  Wrapper<T> operator op(const U& t1, const Wrapper<T>& t2) {                                      \
    return t1 op t2.t;                                                                             \
  }                                                                                                \
                                                                                                   \
  template <typename T, typename U>                                                                \
  Wrapper<T> operator op(const Wrapper<T>& t1, const U& t2) {                                      \
    return t1.t op t2;                                                                             \
  }

WRAP_ARITHMETIC_OPS(+)
WRAP_ARITHMETIC_OPS(-)
WRAP_ARITHMETIC_OPS(*)
WRAP_ARITHMETIC_OPS(/)
WRAP_ARITHMETIC_OPS(%)

template <typename T>
Wrapper<T> abs(const Wrapper<T>& in) {
  return Wrapper<T>(abs(in.t));
}

namespace std {

template <typename T>
struct hash<Wrapper<T>> {
  size_t operator()(const Wrapper<T>& x) const {
    return getHash(x.t);
  }
};
} // namespace std

template <typename T>
std::ostream& json_dump(const Wrapper<T>& t, std::ostream& o) {
  return json_dump(t.t, o);
}

#endif // _WRAPPER_H
