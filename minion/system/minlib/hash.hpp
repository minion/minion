#ifndef HASH_HPP_XAXC
#define HASH_HPP_XAXC

#include "basic_sys.hpp"
#include <unordered_set>

template <typename Arg>
struct minlib_hash_base {
  typedef size_t result_type;
  typedef Arg argument_type;
};

template <typename T>
size_t getHash(const T& t) {
  return std::hash<T>()(t);
}

template <typename T, typename U>
size_t hashCombine(const T& t, const U& u) {
  return getHash(t) * 443782 + getHash(u) * 30193;
}

namespace std {
template <typename T, typename U>
struct hash<std::pair<T, U>> : minlib_hash_base<std::pair<T, U>> {
public:
  size_t operator()(const std::pair<T, U>& p) const {
    return hashCombine(p.first, p.second);
  }
};
}

#endif
