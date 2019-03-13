#include "minlib/minlib.hpp"

int main(void) {
  std::array<int, 3> a1 = make_arrayWith_common_type<char*>(1, 2, 3);
  D_ASSERT(a1[0] == 1 && a1[1] == 2 && a1[2] == 3);

  std::array<float, 3> a2 = make_arrayWith_common_type<float>(1, 2, 'c');
  D_ASSERT(a2[0] == 1 && a2[1] == 2 && a2[2] == 'c');
}