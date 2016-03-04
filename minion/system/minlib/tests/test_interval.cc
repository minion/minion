#include "minlib/minlib.hpp"

int main(void) {
  D_ASSERT(!is_interval<decltype(1)>::val);
  D_ASSERT(!is_interval<decltype("bob")>::val);
  D_ASSERT(is_interval<decltype(make_interval(1, 2))>::val);
}
