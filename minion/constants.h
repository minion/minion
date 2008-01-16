/* Minion
* Copyright (C) 2006
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

/// A placeholder type.
struct EmptyType
{};

/// A big constant, when such a thing is needed.
const int big_constant = 999999;


/// A constant chosen at compile time.
/// Create with the notation compiletime_val<6>().
template<int i>
struct compiletime_val
{ 
  int val() {return i; }
  template<int j>
  compiletime_val<i+j> add()
{ return compiletime_val<i+j>(); }
};


/// A constant chosen at run time.
/// Create with the notation runtime_val(6).
struct runtime_val
{
  int i;
  runtime_val(int _i) : i(_i)
  {}
  int val() { return i; }
  
  template<int j>
    runtime_val add()
  { return runtime_val(i+j); }
};

template<typename T1, typename T2>
inline int mymin(T1 t1, T2 t2)
{
  if(t1 <= t2)
    return t1;
  else
    return t2;
}

template<typename T1, typename T2>
inline int mymax(T1 t1, T2 t2)
{
  if(t1 <= t2)
    return t2;
  else
    return t1;
}

enum TrigType
{ 
  UpperBound, 
  LowerBound, 
  Assigned,
  DomainChanged, 
  DomainRemoval
};

enum BoundType
{
  Bound_Yes,
  Bound_No,
  Bound_Maybe
};

