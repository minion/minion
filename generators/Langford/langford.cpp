#include<iostream>
#include<string>
#include<stdio.h>
#include<stdlib.h>

using namespace std;

string types[3] = { "element", "gacelement", "watchelement" };

int main(int argc, char** argv)
{

  if(argc < 2) {
	   cerr << " usage: langford k type  "
		<< endl
	        << "\t type: 0: element,  1: gacelement,  2: watchelement"
	        << endl;  
	   return 0;
   }
  
  int k = atoi(argv[1]);
  int type = atoi(argv[2]);
  
  int n = 2;
  cout << "MINION 1" << endl;
  cout << "# Langford's problem : L(" << k << "," << n << ")" << endl;
  cout << "0 0 0" << endl;
  cout << k*n + k*2 << endl;
  cout << 1 << " " << k << " " << k*n << endl;
  cout << 0 << " " << k*n << " " << k*2 << endl;
  cout << 0 << endl;
  cout << "[x0";
  for(int i = 1; i < k*n + k*2; ++i)
   cout << ",x" << i;
  cout << "]" << endl;
  
  cout << "[a";
  for(int i = 1; i < k*n + k*2; ++i)
   cout << ",a";
  cout << "]" << endl;
  
  cout << 0 << endl;
  cout << 1 << endl;
  cout << "[[x0";
  for(int i = 1; i < k*n + k*2; ++i)
   cout << ",x" << i;
  cout << "]]" << endl;
  
  cout << 0 << endl;
  cout << "objective none" << endl;
  cout << "print m0" << endl;
  
  for(int i = 0; i < k; ++i)
  {
    cout << types[type] << "(";
    cout << "[x0";
    for(int j = 1; j < k*n; ++j)
      cout << ",x" << j;
    cout << "]";
  
    cout << ",x" << k*n + 2*i << "," << i + 1 << ")" << endl;
  
    cout << types[type] << "(";
    cout << "[x0";
    for(int j = 1; j < k*n; ++j)
      cout << ",x" << j;
    cout << "]";
  
    cout << ",x" << k*n + 2*i + 1 << "," << i + 1 << ")" << endl;
  
    printf("sumleq([x%d, %d], x%d)\n", k*n + 2*i, i+2, k*n + 2*i + 1);
    printf("sumgeq([x%d, %d], x%d)\n", k*n + 2*i, i+2, k*n + 2*i + 1);
  }
}

