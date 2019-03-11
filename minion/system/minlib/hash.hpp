#ifndef HASH_HPP_XAXC
#define HASH_HPP_XAXC

#include "basic_sys.hpp"
#include <unordered_set>

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
struct hash<std::pair<T, U>> {
public:
  size_t operator()(const std::pair<T, U>& p) const {
    return hashCombine(p.first, p.second);
  }
};
} // namespace std

#endif
