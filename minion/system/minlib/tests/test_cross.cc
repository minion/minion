#include "minlib/minlib.hpp"

using namespace std;

int main(void) {
  auto vecvec1 = makeVec(makeVec(1, 2), makeVec(3, 4));
  auto vecvec2 = makeVec(makeVec(1, 3), makeVec(1, 4), makeVec(2, 3), makeVec(2, 4));
  D_ASSERT(cross_prod(vecvec1) == vecvec2);

  vecvec1.push_back(vector<int>());
  D_ASSERT(cross_prod(vecvec1) == vector<vector<int>>());

  vector<vector<int>> vecvec3;

  D_ASSERT(cross_prod(vecvec3) == vecvec3);

  auto vecvec4 = makeVec(makeVec(1, 2), vector<int>(), makeVec(3, 4));

  D_ASSERT(cross_prod(vecvec4) == vecvec3);
}
