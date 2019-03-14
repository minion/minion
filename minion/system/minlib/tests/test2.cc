#include "minlib/minlib.hpp"

using namespace std;

int main(void) {
  auto con = make<vector>(1, 2, 3);
  auto convec = makeVec(1, 2, 3);

  D_ASSERT(con == convec);

  int a[3] = {1, 2, 3};
  std::vector<int> vec(a, a + 3);

  D_ASSERT(con == vec);

  D_ASSERT("2" == tostring(2));
  D_ASSERT("[1,2,3]" == tostring(con));

  auto vecvec1 = makeVec(makeVec(1, 2), makeVec(3, 4));
  auto vecvec2 = makeVec(makeVec(1, 3), makeVec(1, 4), makeVec(2, 3), makeVec(2, 4));
  D_ASSERT(cross_prod(vecvec1) == vecvec2);
}
