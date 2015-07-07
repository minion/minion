#include "minlib/minlib.hpp"

using namespace std;

int main(void)
{
    auto vecvec1 = make_vec(make_vec(1,2),make_vec(3,4));
    auto vecvec2 = make_vec(make_vec(1,3),make_vec(1,4),
                            make_vec(2,3),make_vec(2,4));
    D_ASSERT(cross_prod(vecvec1) == vecvec2);

    vecvec1.push_back(vector<int>());
    D_ASSERT(cross_prod(vecvec1) == vector<vector<int>>());

    vector<vector<int> > vecvec3;

    D_ASSERT(cross_prod(vecvec3) == vecvec3);

    auto vecvec4 = make_vec(make_vec(1,2), vector<int>(), make_vec(3,4));

    D_ASSERT(cross_prod(vecvec4) == vecvec3);
}
