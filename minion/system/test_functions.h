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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
* USA.
*/

#ifndef TEST_FUNCTIONS_H
#define TEST_FUNCTIONS_H

#include <string>
#include <sstream>

#include <vector>

template <typename Var>
void add_var_dom_to_json(Var& v, std::ostream& s) {
  if(v.isAssigned() || v.isBound()) {
    s << "[ [" << v.getMin() << "," << v.getMax() << "] ]";
  } else {
      s << "[ ";
      DomainInt lower_range = v.getMin();
      bool in_range = true;
      for(DomainInt i = v.getMin() + 1; i < v.getMax(); ++i) {
        if(in_range) {
          if(!v.inDomain(i)) {
            s << "[" << lower_range << "," << i-1 << "],";
            in_range = false;
          }
        }
        else {
          if(v.inDomain(i)) {
            lower_range = i;
            in_range = true;
          }
        }
      }
      
      if(in_range) {
        s << "[" << lower_range << "," << v.getMax() << "] ]";
      }
      else {
        s << "[" << v.getMax() << "," << v.getMax() << "] ]";
      }
  }
}

template <typename T>
raw_json get_dom_as_json(vector<T>& vec) {
  std::ostringstream s;
  s << "[";
  if(!vec.empty()) {
    add_var_dom_to_json(vec[0], s);
    for(UnsignedSysInt i = 1; i < vec.size(); ++i) {
      s << ", ";
      add_var_dom_to_json(vec[i], s);
    }
  }
  s << "]";
  return raw_json(s.str());
}

template <typename Var>
string get_dom_as_string(Var& v) {
  ostringstream s;
  if(v.isAssigned()) {
    s << v.getAssignedValue();
  } else {
    if(v.isBound()) {
      s << "[" << v.getMin() << "," << v.getMax() << "]";
    } else {
      s << "{" << v.getMin();
      for(DomainInt i = v.getMin() + 1; i <= v.getMax(); ++i)
        if(v.inDomain(i))
          s << "," << i;
      s << "}";
    }
  }
  return s.str();
}

template <typename T>
string get_dom_as_string(vector<T>& vec) {
  string output("<");
  if(!vec.empty()) {
    output += get_dom_as_string(vec[0]);
    for(UnsignedSysInt i = 1; i < vec.size(); ++i) {
      output += ",";
      output += get_dom_as_string(vec[i]);
    }
  }
  output += ">";
  return output;
}

// Count number of literals present in an array of variables.
template <typename Vars>
DomainInt lit_count(Vars& v) {
  DomainInt lits = 0;
  for(SysInt i = 0; i < (SysInt)v.size(); ++i) {
    if(v[i].isBound()) {
      lits += v[i].getMax() - v[i].getMin() + 1;
    } else {
      for(DomainInt j = v[i].getMin(); j <= v[i].getMax(); ++j)
        if(v[i].inDomain(j))
          lits++;
    }
  }
  return lits;
}

#endif
