#include "minlib/minlib.hpp"

int main(void) {
  MultiDimCon<int, int> m(makeVec(2, 2, 2));
  D_ASSERT(m.arity() == 3);
  MultiDimCon<int, int> p = project(m, 1, 1, 1);
  D_ASSERT(p.arity() == 0);
  D_THROWS(project(m, 1, 1));
  D_THROWS(project(m, 1, 1, 1, 1));

  m.add(makeVec(1, 1, 1), 3);
  m.add(makeVec(0, 0, 0), 6);
  m.add(makeVec(0, 0, 1), 5);

  D_ASSERT(m.arity() == 3);
  p = project(m, 1, 1, 1);

  D_ASSERT(p.get(makeVec<int>()) == 3);

  MultiDimCon<int, int> X(project(m, 0, 0, 0));
}
