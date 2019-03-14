#include "minlib/minlib.hpp"

int main(void) {
  INTERVAL<int> i(make_interval(1, 2)), j(make_interval(3, 3)), k(make_interval(3, 4));

  auto l1 = intervalise<int>(i);
  assert(l1 == i);

  auto l2 = intervalise<int>(3);
  assert(l2 == j);

  auto l3 = intervalise_list<int>(i);
  assert(l3 == makeVec(i));

  auto l4 = intervalise_list<int>(3);
  assert(l4 == makeVec(j));

  auto l5 = intervalise_list<int>(3, i, k);
  assert(l5 == makeVec(j, i, k));
}
