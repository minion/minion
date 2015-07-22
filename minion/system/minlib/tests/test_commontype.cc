#include "minlib/minlib.hpp"

void f(char *);

int main(void) {
  std::is_same<int, CommonType<int, int>::type> T1;
  D_ASSERT(T1);

  std::is_same<double, CommonType<int, double>::type> T2;
  D_ASSERT(T2);

  std::is_same<double, CommonType<int, double, double>::type> T3;
  D_ASSERT(T3);

  std::is_same<int, CommonType<int, double, float>::type> T4;
  D_ASSERT(T4);
}
