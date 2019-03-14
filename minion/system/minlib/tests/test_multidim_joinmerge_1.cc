#include "minlib/minlib.hpp"

int main(void) {
  MultiDimCon<int, int> m(makeVec(2, 2, 2));

  m.add(makeVec(1, 1, 1), 3);
  m.add(makeVec(0, 0, 0), 6);
  m.add(makeVec(0, 0, 1), 5);

  MultiDimCon<int, int> q(makeVec(2, 2, 2));

  q.add(makeVec(1, 1, 1), 3);
  q.add(makeVec(0, 0, 0), 6);
  q.add(makeVec(0, 0, 1), 5);

  auto mer(mdc_join_and_merge(makeVec(m, q)));

  D_ASSERT(mer == makeVec(6, 5, 3, 6, 5, 3));
}
