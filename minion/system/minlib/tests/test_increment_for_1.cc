#include "minlib/minlib.hpp"

using namespace std;

int main(void) {
  vector<vector<int>> vecs;
  for(auto v : ContainerRange(makeVec(2, 1, 2))) {
    vecs.push_back(v);
  }

  D_ASSERT(vecs.size() == 4);
  D_ASSERT(vecs[0] == makeVec(0, 0, 0));
  D_ASSERT(vecs[1] == makeVec(0, 0, 1));
  D_ASSERT(vecs[2] == makeVec(1, 0, 0));
  D_ASSERT(vecs[3] == makeVec(1, 0, 1));
}
