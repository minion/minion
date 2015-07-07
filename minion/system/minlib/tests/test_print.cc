#include "minlib/minlib.hpp"

using namespace std;

int main(void)
{
	auto pa = make_pair(make_pair(1,2),3);
	D_ASSERT("((1,2),3)" == tostring(pa));
    auto con = make<vector>(1,2,3);
    D_ASSERT("2" == tostring(2));
    D_ASSERT("[1,2,3]" == tostring(con));
    auto p = make_pair(con, 2);
    D_ASSERT("([1,2,3],2)" == tostring(p));
    D_ASSERT("bob" == tostring("bob"));
    MAKE_STACK_BOX(a, int, 5);
    a.push_back(1);
    a.push_back(2);

    option<int> i;
    D_ASSERT("<empty>" == tostring(i));
    i = 2;
    D_ASSERT("2" == tostring(i));
    D_ASSERT("[1,2]" == tostring(a));
}
