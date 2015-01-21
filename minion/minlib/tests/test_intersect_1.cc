#include "minlib/minlib.hpp"

int main(void)
{
	auto s1 = make_set<int>(1,2,3);
	auto s2 = make_set<int>(2,4);
	auto s3 = make_set<int>();

	assert( make_set<int>(2) == set_intersect(s1,s2) );
	assert( make_set<int>() == set_intersect(s1,s3) );
}