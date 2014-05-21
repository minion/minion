#include "minlib/minlib.hpp"

int main(void)
{
    MultiDimCon<int, int> m(make_vec(2,2,2));

    m.add(make_vec(1,1,1), 3);
    m.add(make_vec(0,0,0), 6);
    m.add(make_vec(0,0,1), 5);

    MultiDimCon<int, int> q(make_vec(2,2,2));

    q.add(make_vec(1,1,1), 3);
    q.add(make_vec(0,0,0), 6);
    q.add(make_vec(0,0,1), 5);

    auto mer(mdc_join_and_merge(make_vec(m,q)));

    D_ASSERT(mer == make_vec(6,5,3,6,5,3));
}
