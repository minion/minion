#include "minlib/minlib.hpp"

int main(void) {
  auto s1 = makeSet<int>(1, 2, 3);
  auto s2 = makeSet<int>(2, 4);
  auto s3 = makeSet<int>();

  assert(makeSet<int>(2) == set_intersect(s1, s2));
  assert(makeSet<int>() == set_intersect(s1, s3));
}