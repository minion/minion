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
    bool operator()(array<int, 7> a)
    {
        return ((a[0]+ a[1]+a[2]+a[3]+a[4]+a[5]) > 0) == a[6];
    }
    
    static const int vcount = 7;
    static const int domsize = 2;
};

struct pegsol
{
    bool operator()(array<int, 7> a)
    {
        if( a[0] && !a[1] && a[2] && !a[3] && !a[4] && a[5])
        {
            return a[6];
        }
        else
        {
            return !a[6];
        }
    }
    
    static const int vcount=7;
    static const int domsize=2;
};


struct readfile
{
    static const int vcount = 5;
    static const int domsize = 2;

    vector<array<int, vcount>> v;

    bool operator()(const array<int, vcount>& a)
    {
        return std::binary_search(v.begin(), v.end(), a);
    }
};


struct sokoban
{
    bool operator()(array<int, 3> a)
    {
        switch(a[1])
        {
        case 0:
            return a[0]-2==a[2];
        case 1:
            return a[0]-1==a[2];
        case 2:
            return a[0]+1==a[2];
        case 3:
            return a[0]+2==a[2];
        default:
            cout << "Wotcher playing at"<<endl;
            abort();
        }
    }
    
    static const int vcount=3;
    static const int domsize=4;
};

struct Life
{
    bool operator()(array<int, 10> a)
    {
        int sum = 0;
        for(int i = 0; i < 8; ++i)
            sum += a[i];

        if( ((sum < 2) || (sum > 3)) && a[9] == 0)
            return true;

        if( ( sum == 3) && a[9] == 1)
            return true;

        if( (sum == 2) && (a[8] == a[9]) )
            return true;
        return false;
    }

    static const int vcount = 10;
    static const int domsize = 2;
};

//typedef ReifyDiffEqualConCon CurrentConstraint;
//typedef EqualReifyNotEqual CurrentConstraint;
typedef Life CurrentConstraint;

static const int vcount = CurrentConstraint::vcount;
static const int domsize = CurrentConstraint::domsize;
static const int dcount = domsize * vcount;


