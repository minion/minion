/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

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

template<typename Type>
class Reversible
{
  BackTrackOffset backtrack_ptr;
  
public:
  
  
  operator Type() const
{  
  Type* ptr = (Type*)(backtrack_ptr.get_ptr());
  return *ptr;
}

  void operator=(const Type& newval)
  {
    Type* ptr = (Type*)(backtrack_ptr.get_ptr());
    *ptr = newval;
  }

  void operator++()
  { *this = *this + 1; }

  void operator--()
  { *this = *this - 1; }
  

Reversible()
{ 
   backtrack_ptr.request_bytes(sizeof(Type));
   D_ASSERT( (size_t)(backtrack_ptr.get_ptr()) % sizeof(Type) == 0);
}

  friend std::ostream& operator<<(std::ostream& o, const Reversible& v)
  { return o << Type(v); }

};

typedef Reversible<int> ReversibleInt;

