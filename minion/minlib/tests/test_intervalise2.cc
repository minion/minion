#include "minlib/minlib.hpp"

int main(void)
{
	INTERVAL<int> i(make_interval(1,2)),
								j(make_interval(3,3)),
								k(make_interval(3,4));

	INTERVAL_SET<int> S;

	is_insert(S,i);
	is_insert(S,j);

	assert(contains(S, make_interval(1,3)));
	assert(!contains(S, make_interval(1,4)));
	assert(!contains(S,0));
	assert(contains(S, 1));
	assert(contains(S, 2));
	assert(contains(S, 3));
	assert(!contains(S, 4));

	is_insert(S, k);
	assert(contains(S, make_interval(1,4)));
}
