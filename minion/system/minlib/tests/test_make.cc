#include "minlib/minlib.hpp"

using namespace std;

int main(void) {
  auto con = make<vector>(1, 2, 3);
  auto convec = make_vec(1, 2, 3);

  D_ASSERT(con == convec);

  int a[3] = {1, 2, 3};
  std::vector<int> vec(a, a + 3);

  D_ASSERT(con == vec);
}
