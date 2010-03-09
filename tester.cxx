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
    cout << "mytab 4 3 0 0 0 0 1 0 1 0 0 1 1 1" << endl;
    cout << "**VARIABLES**" << endl;
    cout << "BOOL v[" << num << "]" << endl;
    cout << "**CONSTRAINTS**" << endl;
  //  cout << "sumleq(v, 50)" << endl;
  //  cout << "sumgeq(v, 51)" << endl;

    for(int i = 0; i < 2000; ++i)
    {
        int x1,x2,x3;
        x1 = rand() % num;
        x2 = rand() % num;
        x3 = rand() % num;


        switch(choice)
        {
            case 0:
        printf("test([v[%d],v[%d],v[%d]])\n", x1, x2, x3);
        break;
            case 1:
        printf("table([v[%d],v[%d],v[%d]], mytab)\n", x1 ,x2, x3);
        break;
            case 2:
        printf("min([v[%d],v[%d]], v[%d])\n", x1, x2, x3);
        break;
           case 3:
        printf("product(v[%d],v[%d], v[%d])\n", x1, x2, x3);
        break;
        default: abort();
        }
    }
    cout << "**EOF**" << endl;


}
