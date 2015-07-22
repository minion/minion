#ifndef OPTIONAL_HPP
#define OPTIONAL_HPP

#include "macros.hpp"

template <typename T>
class option {
  T t;
  bool present;

  typedef void (option::*bool_type)() const;
  void random_function_name() const {}

public:
  operator bool_type() const {
    bool_type tr = &option::random_function_name;
    bool_type f(0);
    return present ? tr : f;
  }

  void clear() { present = false; }

  T &operator*() {
    D_ASSERT(present);
    return t;
  }

  const T &operator*() const {
    D_ASSERT(present);
    return t;
  }

  option() : present(false) {}

  option(const T &_t) : t(_t), present(true) {}

  option(const option &o) : t(o.t), present(o.present) {}
};

template <typename T>
bool operator==(const option<T> &lhs, const option<T> &rhs) {
  if (!lhs || !rhs)
    return (bool)lhs == (bool)rhs;
  else
    return (*lhs) == (*rhs);
}

template <typename T>
bool operator!=(const option<T> &lhs, const option<T> &rhs) {
  return !(lhs == rhs);
}

template <typename T>
bool operator<(const option<T> &lhs, const option<T> &rhs) {
  if ((bool)lhs != (bool)rhs) {
    return (bool)lhs < (bool)rhs;
  }

  if (lhs && rhs) {
    return *lhs < *rhs;
  }

  // both objects are disengaged.
  return false;
}

inline option<bool> option_and(option<bool> lhs, option<bool> rhs) {
  if ((bool)lhs && !(*lhs))
    return option<bool>(false);

  if ((bool)rhs && !(*rhs))
    return option<bool>(false);

  if ((bool)lhs && *lhs && (bool)rhs && *rhs)
    return option<bool>(true);

  return option<bool>();
}

inline option<bool> option_or(option<bool> lhs, option<bool> rhs) {
  if ((bool)lhs && (*lhs))
    return option<bool>(true);

  if ((bool)rhs && (*rhs))
    return option<bool>(true);

  if ((bool)lhs && !(*lhs) && (bool)rhs && !(*rhs))
    return option<bool>(false);

  return option<bool>();
}

#endif
