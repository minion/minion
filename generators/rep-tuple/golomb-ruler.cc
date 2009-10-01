#include<algorithm>
#include<vector>
#include<iostream>
#include<cmath>
#include<tr1/tuple>

using namespace std;
using namespace tr1;

int main(int argc, char** argv)
{
  int ticks = atoi(argv[1]);
  int size = atoi(argv[2]);
  
  cout << "MINION 1" << endl;
  cout << "# Tuple-based Golomb Ruler" << endl;
  cout << "# ticks = " << ticks << endl;
  cout << "# size = " << size << endl;
  
  cout << "0 0 0" << endl;
  cout << ticks << endl;
  cout << 0 << " " << size << " " << ticks << endl;
  cout << "0" << endl;
  cout << "[] []" << endl;
  cout << "1" << endl;
  cout << "[x0";
  for(int i = 1; i < ticks; ++i)
    cout << ",x" << i;
  cout << "]" << endl;
  
  cout << "0 0" << endl;
  cout << "tuplelists " << 2 << endl;
  
  
  // Handle triples: <a,b,c> : |a - b| =\= |b - c|
  vector<tuple<int, int, int> > con3;
  
  for(int a = 0; a < size; ++a)
    for(int b = a + 1; b < size; ++b)
      for(int c = b + 1; c < size; ++c)
      {
        if( abs(b - a) != abs(c - b) )
          con3.push_back(make_tuple(a,b,c));
      }

  cout << con3.size() << " " << 3 << endl;
  for(int i = 0; i < con3.size(); ++i)
  {
    cout << get<0>(con3[i]) << " "
    	 << get<1>(con3[i]) << " "
    	 << get<2>(con3[i]) << endl;
  }
  
  
   // Handle 4s: <a,b,c,d> : |a - b| =\= |c - d|
  vector<tuple<int, int, int, int> > con4;
  
  for(int a = 0; a < size; ++a)
    for(int b = 0; b < size; ++b)
      for(int c = 0; c < size; ++c)
        for(int d = 0; d < size; ++d)
        {
          if( abs(b - a) != abs(d - c) )
            con4.push_back(make_tuple(a,b,c,d));
        }

  cout << con4.size() << " " << 4 << endl;
  for(int i = 0; i < con4.size(); ++i)
  {
    cout << get<0>(con4[i]) << " "
    	 << get<1>(con4[i]) << " "
    	 << get<2>(con4[i]) << " "
    	 << get<3>(con4[i]) << endl;
  }

  cout << "objective none" << endl;
  cout << "print v0" << endl;
  
  for(int a = 0; a < ticks; ++a)
    for(int b = a + 1; b < ticks; ++b)
      for(int c = b + 1; c < ticks; ++c)
        printf("table([x%d,x%d,x%d],t0)\n",a,b,c);
  
  for(int a = 0; a < ticks; ++a)
    for(int b = a + 1; b < ticks; ++b)
      for(int c = a + 1; c < ticks; ++c)
        for(int d = c + 1; d < ticks; ++d)
        {
          if(a != c && a != d && b != c && b != d)
            printf("table([x%d,x%d,x%d,x%d],t1)\n",a,b,c,d);
        }
        
  
}