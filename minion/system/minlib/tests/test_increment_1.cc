#include "minlib/minlib.hpp"

using namespace std;

int main(void) {
  auto maxv = makeVec(2, 1, 2);

  auto v1 = makeVec(0, 0, 0);

  D_ASSERT(incrementVector(v1, maxv));
  D_ASSERT(v1 == makeVec(0, 0, 1));
  D_ASSERT(incrementVector(v1, maxv));
  D_ASSERT(v1 == makeVec(1, 0, 0));
  D_ASSERT(incrementVector(v1, maxv));
  D_ASSERT(v1 == makeVec(1, 0, 1));
  D_ASSERT(!incrementVector(v1, maxv));
  D_ASSERT(v1 == makeVec(0, 0, 0));
}
