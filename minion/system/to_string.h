/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef TOSTRING_H
#define TOSTRING_H

#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <list>
#include <ostream>
#include <sstream>


template<typename T, typename U>
std::ostream& operator<<(std::ostream& o, const std::pair<T,U>& p)
{ return o << "(" << p.first << "," << p.second << ")"; }

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

template<typename T>
std::ostream& operator<<(std::ostream& o, const std::set<T>& t)
{
  output_container(o, t);
  return o;
}

template<typename T>
std::ostream& operator<<(std::ostream& o, const std::multiset<T>& t)
{
  output_container(o, t);
  return o;
}

template<typename T>
T from_string_checked(const std::string& str)
{
  std::istringstream iss(str);
  T obj;
  iss >> std::ws >> obj >> std::ws;
  if(!iss.eof()) throw "Failed Conversion";
  return obj; 
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
  std::ostringstream streamOut;
  streamOut << t1 << " " << t2;
  return streamOut.str();
}

#endif
