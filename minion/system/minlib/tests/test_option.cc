#include "minlib/optional.hpp"

using namespace std;

int main(void)
{
	option<bool> a[3];
	a[1] = false;
	a[2] = true;

	for(int i = 0; i < 3; ++i)
		for(int j = 0; j < 3; ++j)
		{
			if(i == j)
			{
				D_ASSERT(a[i] == a[j]);
				D_ASSERT(!(a[i] != a[j]));
			}
			else
			{
				D_ASSERT(!(a[i] == a[j]));
				D_ASSERT(a[i] != a[j]);
			}
		}

	D_ASSERT(option_or(a[0], a[1]) == a[0]);
	D_ASSERT(option_or(a[0], a[2]) == a[2]);
	D_ASSERT(option_or(a[1], a[2]) == a[2]);

	D_ASSERT(option_and(a[0], a[1]) == a[1]);
	D_ASSERT(option_and(a[0], a[2]) == a[0]);
	D_ASSERT(option_and(a[1], a[2]) == a[1]);

	for(int i = 0; i < 3; ++i)
	{
		D_ASSERT(option_and(a[i], a[i]) == a[i]);
		D_ASSERT(option_or(a[i], a[i]) == a[i]);
	}
}
