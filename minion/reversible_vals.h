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

// \addtogroup Memory
// @{

/// Provides a wrapper around a single backtrackable value.
/** This class aims to act like a normal isntance of 'Type', but is
 *  backtracked.
 */
template<typename Type>
class Reversible
{
  MoveablePointer backtrack_ptr;
  
public:
  
  /// Automatic conversion to the type.
  operator Type() const
  {  
    Type* ptr = (Type*)(backtrack_ptr.get_ptr());
    return *ptr;
  }

  /// Assignment operator.
  void operator=(const Type& newval)
  {
    Type* ptr = (Type*)(backtrack_ptr.get_ptr());
    *ptr = newval;
  }
  
  void operator++()
  { *this = *this + 1; }

  void operator--()
  { *this = *this - 1; }
  
  /// Constructs a new backtrackable type connected to stateObj.
  Reversible(StateObj* stateObj)
  { 
     backtrack_ptr = getMemory(stateObj).backTrack().request_bytes(sizeof(Type));
     D_ASSERT( (size_t)(backtrack_ptr.get_ptr()) % sizeof(Type) == 0);
  }

  /// Provide output.
  friend std::ostream& operator<<(std::ostream& o, const Reversible& v)
  { return o << Type(v); }

};

/// Specialisation for backwards compatability.
typedef Reversible<int> ReversibleInt;

// @}
