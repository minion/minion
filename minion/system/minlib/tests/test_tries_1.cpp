#include "minlib/tries.hpp"


template<typename Tuples>
void test_tuples(const Tuples& tuples)
{
	auto ptr = build_trie(tuples);
	auto vec = unroll_trie(ptr);
	D_ASSERT(tuples == vec);

	randomise_trie(ptr);
	auto vec2 = unroll_trie(ptr);
	std::sort(vec2.begin(), vec2.end());
	D_ASSERT(tuples == vec2);
}

int main(void)
{
	test_tuples(make_vec(make_vec(1),make_vec(2),make_vec(3),make_vec(4),make_vec(5)));
	test_tuples(make_vec(make_vec(1,2),make_vec(3,4)));
	test_tuples(make_vec(make_vec(1)));
	test_tuples(make_vec(make_vec(1,2),make_vec(1,3),make_vec(1,5)));
	test_tuples(make_vec(make_vec(1,2,3,4,5,6)));
	test_tuples(make_vec(make_vec(1,1,1,1,1),make_vec(2,0,0,0,0)));
}