#ifndef CONTAINERS_H_FDASJHFD
#define CONTAINERS_H_FDASJHFD

#include "basic_sys.hpp"
#include "macros.hpp"
#include "variadic.hpp"

template <typename T>
void push_back(T&) {}

template <typename T, typename FirstArg>
void push_back(T& t, const FirstArg& fa) {
  t.push_back(fa);
}

template <typename T, typename FirstArg, typename... Args>
void push_back(T& t, const FirstArg& fa, const Args&... args) {
  t.push_back(fa);
  push_back(t, args...);
}

template <typename T>
void do_insert(T&) {}

template <typename T, typename FirstArg>
void do_insert(T& t, const FirstArg& fa) {
  t.insert(fa);
}

template <typename T, typename FirstArg, typename... Args>
void do_insert(T& t, const FirstArg& fa, const Args&... args) {
  t.insert(fa);
  do_insert(t, args...);
}

template <typename T, typename FirstArg>
void fill_container(T& t, int pos, const FirstArg& fa) {
  t[pos] = fa;
  D_ASSERT(pos == (int)t.size() - 1);
}

template <typename T, typename FirstArg, typename... Args>
void fill_container(T& t, int pos, const FirstArg& fa, const Args&... args) {
  t[pos] = fa;
  fill_container(t, pos + 1, args...);
}

template <template <typename...> class T, typename FirstArg, typename... Args>
T<FirstArg> make(const FirstArg& fa, const Args&... args) {
  T<FirstArg> t;
  t.reserve(SizeOf<Args...>::size);
  push_back(t, fa, args...);
  return t;
}

// Needed for windows
template <typename FirstArg, typename... Args>
std::vector<FirstArg> makeVector(const FirstArg& fa, const Args&... args) {
  std::vector<FirstArg> t;
  t.reserve(SizeOf<Args...>::size);
  push_back(t, fa, args...);
  return t;
}

template <typename FirstArg, typename... Args>
std::vector<FirstArg> makeVec(const FirstArg& fa, const Args&... args) {
  std::vector<FirstArg> v;
  v = makeVector(fa, args...);
  return v;
}

template <typename VecArg, typename FirstArg, typename... Args>
std::vector<VecArg> makeVecWith_type(const FirstArg& fa, const Args&... args) {
  return makeVector(static_cast<VecArg>(fa), args...);
}

template <typename Arg, typename... Args>
std::array<Arg, SizeOf<Args...>::size> make_arrayWith_type(const Args&... args) {
  std::array<Arg, SizeOf<Args...>::size> a;
  fill_container(a, 0, args...);
  return a;
}

template <typename VecArg>
std::vector<VecArg> makeVecWith_type() {
  return std::vector<VecArg>();
}

template <typename Type>
std::vector<Type> makeVec() {
  return std::vector<Type>();
}

template <typename Type, typename... Args>
std::vector<typename CommonType<Type, Args...>::type>
makeVecWith_common_type(const Args&... args) {
  return makeVecWith_type<typename CommonType<Type, Args...>::type>(args...);
}

template <typename Type, typename... Args>
std::array<typename CommonType<Type, Args...>::type, SizeOf<Args...>::size>
make_arrayWith_common_type(const Args&... args) {
  return make_arrayWith_type<typename CommonType<Type, Args...>::type>(args...);
}

template <typename T>
void container_push_back(T&) {}

template <typename T, typename Arg1, typename... Args>
void container_push_back(T& t, const Arg1& arg1, const Args&... args) {
  for(auto it = arg1.begin(); it != arg1.end(); ++it)
    t.push_back(*it);
  container_push_back(t, args...);
}

template <typename Con, typename... Args>
std::vector<typename Con::value_type> joinToVec(const Con& vec1, const Args&... args) {
  std::vector<typename Con::value_type> vec_out(vec1.begin(), vec1.end());
  container_push_back(vec_out, args...);
  return vec_out;
}

template <typename FirstArg, typename... Args>
std::set<FirstArg> makeSet(const FirstArg& fa, const Args&... args) {
  std::set<FirstArg> s;
  do_insert(s, fa, args...);
  return s;
}

template <typename Type>
std::set<Type> makeSet() {
  return std::set<Type>();
}

template <template <typename...> class OutCon, typename InCon>
OutCon<typename InCon::value_type> to_container(const InCon& c) {
  return OutCon<typename InCon::value_type>(c.begin(), c.end());
}

template <typename InCon>
std::vector<typename InCon::value_type> toVec(const InCon& c) {
  return std::vector<typename InCon::value_type>(c.begin(), c.end());
}

template <typename InCon>
std::set<typename InCon::value_type> to_set(const InCon& c) {
  return std::set<typename InCon::value_type>(c.begin(), c.end());
}

template <typename Elem>
std::set<Elem> set_intersect(const std::set<Elem>& s1, const std::set<Elem>& s2) {
  std::set<Elem> intersect;

  std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(),
                        std::insert_iterator<std::set<Elem>>(intersect, intersect.begin()));

  return intersect;
}

template <typename Con, typename Val>
bool unordered_contains(const Con& con, const Val& val) {
  for(auto it = con.begin(); it != con.end(); ++it)
    if(*it == val)
      return true;
  return false;
}
#endif
