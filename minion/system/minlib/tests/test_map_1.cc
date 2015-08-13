#include "minlib/minlib.hpp"

using namespace std;

struct MapperObj {
  std::pair<int, int> operator()(int i) {
    return make_pair(i, i);
  }
};

int main(void) {
  std::vector<int> v;
  v.push_back(2);
  std::vector<pair<int, int>> res = doMap(MapperObj(), v);
  D_ASSERT(res == make_vec(make_pair(2, 2)));

  std::set<int> s;
  s.insert(1);
  std::set<pair<int, int>> sp = doMap(MapperObj(), s);
  D_ASSERT(sp.count(make_pair(1, 1)) == 1);
}
