#include "minlib/minlib.hpp"

using namespace std;

int main(void) {
  auto maxv =
      intervalise_list<int>(make_interval(5, 6), 2, make_interval(3, 3), make_interval(-1, 1));

  auto v = initializeVector_from_intervals(maxv);
  D_ASSERT(v == makeVec(5, 2, 3, -1));

  D_ASSERT(incrementVector_from_intervals(v, maxv));
  D_ASSERT(v == makeVec(5, 2, 3, 0));
  D_ASSERT(incrementVector_from_intervals(v, maxv));
  D_ASSERT(v == makeVec(5, 2, 3, 1));
  D_ASSERT(incrementVector_from_intervals(v, maxv));
  D_ASSERT(v == makeVec(6, 2, 3, -1));
  D_ASSERT(incrementVector_from_intervals(v, maxv));
  D_ASSERT(v == makeVec(6, 2, 3, 0));
  D_ASSERT(incrementVector_from_intervals(v, maxv));
  D_ASSERT(v == makeVec(6, 2, 3, 1));
  D_ASSERT(!incrementVector_from_intervals(v, maxv));
  D_ASSERT(v == makeVec(5, 2, 3, -1));
}
