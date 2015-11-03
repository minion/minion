#include "minlib/minlib.hpp"

using namespace std;

int main(void) {
  auto maxv = make_vec(2, 1, 2);

  auto v1 = make_vec(0, 0, 0);

  D_ASSERT(increment_vector(v1, maxv));
  D_ASSERT(v1 == make_vec(0, 0, 1));
  D_ASSERT(increment_vector(v1, maxv));
  D_ASSERT(v1 == make_vec(1, 0, 0));
  D_ASSERT(increment_vector(v1, maxv));
  D_ASSERT(v1 == make_vec(1, 0, 1));
  D_ASSERT(!increment_vector(v1, maxv));
  D_ASSERT(v1 == make_vec(0, 0, 0));
}
