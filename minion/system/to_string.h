/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#ifndef TOSTRING_H
#define TOSTRING_H

#include <string>
#include <iostream>
#include <vector>
#include <ostream>
#include <list>
#include <sstream>

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
  std::ostringstream streamOut;
  streamOut << t1 << " " << t2;
  return streamOut.str();
}

template<typename T, typename U>
std::ostream& operator<<(std::ostream& o, const std::pair<T,U>& p)
{ o << "pair(" << p.first << "," << p.second << ")"; }

template<typename T>
void output_container(std::ostream& o, const T& t)
{
  o << "[";
  if(!t.empty())
  {
    typename T::const_iterator it(t.begin());
    o << *it;
    ++it;
    for(; it != t.end(); ++it)
      o << " , " << *it; 
  }
  o << "]";
}


template<typename T>
std::ostream& operator<<(std::ostream& o, const std::vector<T>& t)
{
  output_container(o, t);
  return o;
}

template<typename T>
std::ostream& operator<<(std::ostream& o, const std::list<T>& t)
{
  output_container(o, t);
  return o;
}

#endif
