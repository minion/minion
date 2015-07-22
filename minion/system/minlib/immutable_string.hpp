#ifndef IMMUTABLE_STRING_HPP
#define IMMUTABLE_STRING_HPP

#include "basic_sys.hpp"
#include "hash.hpp"

class ImmutableStringCache {
public:
  static ImmutableStringCache &getInstance() {
    static ImmutableStringCache instance;

    return instance;
  }

private:
  ImmutableStringCache(){};
  ImmutableStringCache(ImmutableStringCache const &); // Don't Implement
  void operator=(ImmutableStringCache const &);       // Don't implement

  std::set<std::string> strings;
  // we special case this, as we need it often
  std::string empty_string;

public:
  const std::string *cachedString(const std::string &in) {
    if (in == empty_string)
      return &empty_string;

    auto it = strings.find(in);
    if (it != strings.end())
      return &*it;
    strings.insert(in);

    it = strings.find(in);
    if (it != strings.end())
      return &*it;

    abort();
  }

  const std::string *cachedEmptyString() { return &empty_string; }
};

class ImmutableString {
  // In the long term, this could be made more efficient
  const std::string *ptr;

public:
  const std::string &getStdString() const { return *ptr; }

  // The reason these are explicit is because constructing an 'ImmutableString'
  // is fairly
  // expensive, so we want to know when we do it.

  explicit ImmutableString(const std::string &s)
      : ptr(ImmutableStringCache::getInstance().cachedString(s)) {}

  explicit ImmutableString(const char *s)
      : ptr(ImmutableStringCache::getInstance().cachedString(s)) {}

  ImmutableString() : ptr(ImmutableStringCache::getInstance().cachedEmptyString()) {}

  char operator[](int i) const { return (*ptr)[i]; }

  auto begin() -> decltype(ptr->begin()) { return ptr->begin(); }

  auto end() -> decltype(ptr->end()) { return ptr->end(); }

  friend bool operator==(const ImmutableString &lhs, const ImmutableString &rhs) {
    return lhs.ptr == rhs.ptr;
  }

  friend bool operator==(const ImmutableString &lhs, const char *c) { return *(lhs.ptr) == c; }

  friend bool operator==(const char *c, const ImmutableString &rhs) { return *(rhs.ptr) == c; }

  // this is the only operator where we really have to compare the true strings
  friend bool operator<(const ImmutableString &lhs, const ImmutableString &rhs) {
    return *(lhs.ptr) < *(rhs.ptr);
  }

  friend bool operator!=(const ImmutableString &lhs, const ImmutableString &rhs) {
    return lhs.ptr != rhs.ptr;
  }

  friend std::ostream &operator<<(std::ostream &o, const ImmutableString &is) {
    return o << *(is.ptr);
  }
};

namespace std {
template <>
struct hash<ImmutableString> : minlib_hash_base<ImmutableString> {
public:
  size_t operator()(const ImmutableString &p) const { return getHash(p.getStdString()); }
};
}

#endif
