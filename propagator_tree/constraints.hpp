#include <array>
using namespace std;

struct AndCon
{
    bool operator()(array<int, 3> a)
    {
        return (a[0] && a[1]) == a[2];
    }

    static const int vcount = 3;
    static const int domsize = 2;
};

struct NotAndAndCon
{
    bool operator()(array<int, 4> a)
    {
        return (a[0] && a[1]) != (a[2] && a[3]);
    }
};

struct ReifyDiffEqualConCon
{
    bool operator()(array<int, 5> a)
    {
        int sum = a[1]+a[2]+a[3]+a[4];

        return a[0] || (sum <= 2);
    }

    static const int vcount = 5;
    static const int domsize = 2;
};

struct EqualReifyNotEqual
{
    bool operator()(array<int, 4> a)
    {
        return !(a[0] == a[1]) || (a[2] != a[3]);
    }

    static const int vcount = 4;
    static const int domsize = 2;
};

struct GACSum
{
    bool operator()(array<int, 3> a)
    {

        return (a[0] != a[1]) && (a[0] != a[2]) && (a[1] != a[2]);     
    }

    static const int vcount = 3;
    static const int domsize = 3;
};

struct Eqorneq
{
    bool operator()(array<int, 4> a)
    {
        return (a[0] == a[1]) || (a[2] != a[3]);
    }
    
    static const int vcount = 4;
    static const int domsize = 2;
};

struct sumgeqthree
{
    bool operator()(array<int, 5> a)
    {
        return (a[0]+ a[1]+a[2]+a[3]+a[4])>=3;
    }
    
    static const int vcount = 5;
    static const int domsize = 2;
};

//typedef ReifyDiffEqualConCon CurrentConstraint;
//typedef EqualReifyNotEqual CurrentConstraint;
typedef sumgeqthree CurrentConstraint;

static const int vcount = CurrentConstraint::vcount;
static const int domsize = CurrentConstraint::domsize;
static const int dcount = domsize * vcount;


