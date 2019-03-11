/** \weakgroup MinLib
 * @{
 */
#ifndef TINY_TEMPLATE_DCDSCQ
#define TINY_TEMPLATE_DCDSCQ

#include <ostream>

/// A placeholder type.
struct EmptyType {};

struct AnyGrab {
  template <typename T>
  AnyGrab(const T&);
};

/// A constant chosen at compile time.
/// Create with the notation compiletimeVal<6>().
template <typename T, T i>
struct compiletimeVal {
  operator T() const {
    return i;
  }

  compiletimeVal<T, (T)(-1) - i> negminusone() const {
    return compiletimeVal<T, (T)(-1) - i>();
  }

  friend std::ostream& operator<<(std::ostream& o, const compiletimeVal&) {
    return o << "CompiletimeConst:" << i;
  }

  compiletimeVal<T, (T)0 - i> operator-() const {
    return compiletimeVal<T, (T)0 - i>();
  }
};

template <typename T, T i, T j>
compiletimeVal<T, i + j> operator+(compiletimeVal<T, i>, compiletimeVal<T, j>) {
  return compiletimeVal<T, i + j>();
}

template <typename T, T i, T j>
compiletimeVal<T, i - j> operator-(compiletimeVal<T, i>, compiletimeVal<T, j>) {
  return compiletimeVal<T, i - j>();
}

/** @}
 */

#endif
