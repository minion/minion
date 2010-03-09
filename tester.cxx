#include <iostream>
#include <istream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

int main(int argc, char** argv)
{
    if(argc != 2) abort();
    int choice = atoi(argv[1]);

    const int num = 1000;
    cout << "MINION 3" << endl;
    cout << "**TUPLELIST**" << endl;
    cout << "mytab 12 4 \
        0 1 0 0\
        0 1 0 1\
        0 1 1 0\
        0 1 1 1\
        1 0 0 0\
        1 0 0 1\
        1 0 1 0\
        1 0 1 1\
\
        0 0 1 0\
        1 1 1 0\
        0 0 0 1\
        1 1 0 1" << endl;
    cout << "**VARIABLES**" << endl;
    cout << "BOOL v[" << num << "]" << endl;
    cout << "**CONSTRAINTS**" << endl;
  //  cout << "sumleq(v, 50)" << endl;
  //  cout << "sumgeq(v, 51)" << endl;

    for(int i = 0; i < 10000; ++i)
    {
        int x1,x2,x3,x4;
        x1 = rand() % num;
        x2 = rand() % num;
        x3 = rand() % num;
        x4 = rand() % num;


        switch(choice)
        {
            case 0:
        printf("test([v[%d],v[%d],v[%d], v[%d]])\n", x1, x2, x3, x4);
        break;
            case 1:
        printf("table([v[%d],v[%d],v[%d], v[%d]], mytab)\n", x1 ,x2, x3, x4);
        break;
            case 2:
        printf("watched-or({diseq(v[%d],v[%d]), diseq(v[%d],v[%d])})\n", x1, x2, x3, x4);
            case 3:
        printf("**VARIABLES**\n BOOL x_%d\n BOOL y_%d\n", i, i);
        printf("**CONSTRAINTS**\n");
        printf("reify(diseq(v[%d],v[%d]),x_%d)\n reify(diseq(v[%d],v[%d]), y_%d)\n", x1, x2, i, x3, x4, i);
        printf("sumgeq([x_%d,y_%d],1)\n", i, i);
        break;
        default: abort();
        }
    }
    cout << "**EOF**" << endl;


}
