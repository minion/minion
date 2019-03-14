#include "minlib/minlib.hpp"

int main(void) {
  MultiDimCon<int, int> m(makeVec(2, 2, 2));

  D_ASSERT(!m.exists(makeVec(1, 0, 1)));

  D_THROWS(m.exists(makeVec(3, 1, 1)));
  D_THROWS(m.exists(makeVec(1, 1, 2)));
  D_THROWS(m.exists(makeVec(-1, 1, 1)));

  D_THROWS(m.get(makeVec(1, 1, 1)));
  m.add(makeVec(1, 1, 1), 2);
  D_ASSERT(m.get(makeVec(1, 1, 1)) == 2);
  D_THROWS(m.add(makeVec(1, 1, 1), 2));
  D_THROWS(m.add(makeVec(1, 1, 1), 1));
}
