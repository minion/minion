#include <minlib/minlib.hpp>

void testy(const SimpleMap<int, int>& sm) {
  // test constness
  assert(sm.begin() != sm.end());
}

int main(void) {
  SimpleMap<int, int> sm;

  assert(sm.begin() == sm.end());

  sm.add(1, 1);
  assert(sm.begin() != sm.end());

  SimpleMap<int, int>::iterator it = sm.begin();
  assert(it != sm.end());

  assert(it->first == 1 && it->second == 1);
  it++;
  assert(it == sm.end());

  testy(sm);
};