#include "minlib/minlib.hpp"

struct Thing {};

bool operator==(Thing, Thing) { return true; }

int main(void) {
  MultiDimCon<int, Thing> m(make_vec(2, 2, 2));

  D_ASSERT(!m.exists(make_vec(1, 0, 1)));

  D_THROWS(m.exists(make_vec(3, 1, 1)));
  D_THROWS(m.exists(make_vec(1, 1, 2)));
  D_THROWS(m.exists(make_vec(-1, 1, 1)));

  D_THROWS(m.get(make_vec(1, 1, 1)));
  m.add(make_vec(1, 1, 1), Thing());
  D_ASSERT(m.get(make_vec(1, 1, 1)) == Thing());
  D_THROWS(m.add(make_vec(1, 1, 1), Thing()));
  D_THROWS(m.add(make_vec(1, 1, 1), Thing()));
}
