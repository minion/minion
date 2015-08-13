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
/// Create with the notation compiletime_val<6>().
template <typename T, T i>
struct compiletime_val {
  operator T() const {
    return i;
  }

  compiletime_val<T, (T)(-1) - i> negminusone() const {
    return compiletime_val<T, (T)(-1) - i>();
  }

  friend std::ostream& operator<<(std::ostream& o, const compiletime_val&) {
    return o << "CompiletimeConst:" << i;
  }

  compiletime_val<T, (T)0 - i> operator-() const {
    return compiletime_val<T, (T)0 - i>();
  }
};

template <typename T, T i, T j>
compiletime_val<T, i + j> operator+(compiletime_val<T, i>, compiletime_val<T, j>) {
  return compiletime_val<T, i + j>();
}

template <typename T, T i, T j>
compiletime_val<T, i - j> operator-(compiletime_val<T, i>, compiletime_val<T, j>) {
  return compiletime_val<T, i - j>();
}

/** @}
 */

#endif
