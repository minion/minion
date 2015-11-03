#include "minlib/minlib.hpp"

using namespace std;

int main(void) {
  set<string> s;

  for(int i = 0; i < 10000; ++i)
    s.insert(gensym());

  D_ASSERT(s.size() == 10000);
}
