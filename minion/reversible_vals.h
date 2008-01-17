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
  void set(Type newval)
  {
    Type* int_ptr = (Type*)(backtrack_ptr.get_ptr());
    *int_ptr = newval;
  }

  Type get()
  {
    Type* int_ptr = (Type*)(backtrack_ptr.get_ptr());
    return *int_ptr;
  }

  Type decrement()
  {
      Type* int_ptr = (Type*)(backtrack_ptr.get_ptr());
      return --(*int_ptr)  ;
  }

  Type increment()
  {
      Type* int_ptr = (Type*)(backtrack_ptr.get_ptr());
      return ++(*int_ptr)  ;
  }


Reversible()
{ 
   backtrack_ptr.request_bytes(sizeof(Type));
   D_ASSERT( (size_t)(backtrack_ptr.get_ptr()) % sizeof(Type) == 0);
}

};

typedef Reversible<int> ReversibleInt;

