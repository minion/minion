#include "minlib/minlib.hpp"

template <typename T>
std::vector<int> enumerate_loop(const T& t) {
  std::vector<int> v;
  for(auto i : t)
    v.push_back(i);
  return v;
}

int main(void) {
  assert(make_vec(0) == enumerate_loop(Range(1)));
  assert(make_vec(0, 1) == enumerate_loop(Range(2)));
  assert(make_vec<int>() == enumerate_loop(Range(0)));
  assert(make_vec<int>() == enumerate_loop(Range(-1)));
  assert(make_vec<int>(0, 1, 2) == enumerate_loop(Range(0, 3)));
  assert(make_vec<int>(3, 4) == enumerate_loop(Range(3, 5)));
  assert(make_vec<int>(0, 2) == enumerate_loop(Range(0, 3, 2)));
  assert(make_vec<int>(0, 2) == enumerate_loop(Range(0, 4, 2)));
  assert(make_vec<int>(-3, -2) == enumerate_loop(Range(-3, -1)));
  assert(make_vec<int>(-3, -1, 1) == enumerate_loop(Range(-3, 2, 2)));
  assert(make_vec<int>(-3, -1, 1) == enumerate_loop(Range(-3, 3, 2)));
}