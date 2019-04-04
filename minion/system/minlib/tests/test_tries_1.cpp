#include "minlib/tries.hpp"

template <typename Tuples>
void test_tuples(const Tuples& tuples) {
  auto ptr = buildTrie(tuples);
  auto vec = unrollTrie(ptr);
  D_ASSERT(tuples == vec);

  randomiseTrie(ptr);
  auto vec2 = unrollTrie(ptr);
  std::sort(vec2.begin(), vec2.end());
  D_ASSERT(tuples == vec2);
}

int main(void) {
  test_tuples(makeVec(makeVec(1), makeVec(2), makeVec(3), makeVec(4), makeVec(5)));
  test_tuples(makeVec(makeVec(1, 2), makeVec(3, 4)));
  test_tuples(makeVec(makeVec(1)));
  test_tuples(makeVec(makeVec(1, 2), makeVec(1, 3), makeVec(1, 5)));
  test_tuples(makeVec(makeVec(1, 2, 3, 4, 5, 6)));
  test_tuples(makeVec(makeVec(1, 1, 1, 1, 1), makeVec(2, 0, 0, 0, 0)));
}