/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#ifndef TOSTRING_H
#define TOSTRING_H

#include <string>
#include <sstream>

template<typename T>
std::string
print_vec(T t)
{
  std::ostringstream streamOut;
  streamOut << "(";
  for(unsigned int i=0;i<t.size();i++)
    streamOut << t[i] << ",";
  streamOut << ")";
  return streamOut.str();
}

template<typename T>
std::string
to_string(T t)
{
  std::ostringstream streamOut;
  streamOut << t;
  return streamOut.str();
}

template<typename T1, typename T2>
std::string
to_string(T1 t1, T2 t2)
{
  std::ostringstream streamOut1;
  streamOut1 << t1;
  std::ostringstream streamOut2;
  streamOut2 << t2;
  return streamOut1.str() + ":" + streamOut2.str();
}

template<typename T1, typename T2, typename T3>
std::string
to_string(T1 t1, T2 t2, T3 t3)
{
  std::ostringstream streamOut1;
  streamOut1 << t1;
  std::ostringstream streamOut2;
  streamOut2 << t2;
  std::ostringstream streamOut3;
  streamOut3 << t3;
  
  return streamOut1.str() + ":" + streamOut2.str() + ":" + streamOut3.str();
}

#endif
