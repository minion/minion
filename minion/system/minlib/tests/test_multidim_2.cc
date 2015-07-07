#include "minlib/minlib.hpp"

int main(void)
{
    MultiDimCon<int, int> m(make_vec(2,2,2));
    D_ASSERT(m.arity() == 3);
    MultiDimCon<int, int> p = project(m, 1,1,1);
    p = project(m,1,1,1);
    D_ASSERT(p.arity() == 0);
    D_THROWS(project(m,1,1));
    D_THROWS(project(m,1,1,1,1));

    m.add(make_vec(1,1,1), 3);
    m.add(make_vec(0,0,0), 6);

    D_ASSERT(m.arity() == 3);
    p = project(m, 1, 1, 1);

    D_ASSERT(p.get(make_vec<int>()) == 3);


    MultiDimCon<int,int> p2 = project(m, make_interval(0,1), make_interval(1,1), 1);

    D_ASSERT(p2.get(make_vec(1,0)) == 3);

    MultiDimCon<int,int> f = flatten(m);
    D_ASSERT(f.arity() == 1);
    D_ASSERT(f.get(make_vec(0)) == 6);
    D_ASSERT(f.get(make_vec(1)) == 3);
    D_THROWS(f.get(make_vec(2)));
}
