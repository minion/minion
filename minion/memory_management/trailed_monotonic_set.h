/* $Id$ */

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

#ifndef TRAILED_MONOTONIC_SET_H
#define TRAILED_MONOTONIC_SET_H

typedef bool value_type;

class TrailedMonotonicSet
{
  StateObj* stateObj;

  static const value_type tms_in_set = 1;

  int _size;
  int _max_undos;
  int _max_depth;             

  int _local_depth;             // could be unsigned 
  ReversibleInt _backtrack_depth;

  MemOffset _array;
  MemOffset _undo_indexes;
  
  MoveablePointer backtrack_ptr;

  int& undo_indexes(int i)
  { 
	return static_cast<int*>(_undo_indexes.get_ptr())[i]; 
  }

public:
  // following allows external types destructive changes to array 
  // but we probably do not want to allow this to force them to use set()
  value_type& array(DomainInt i)
  { 
    D_ASSERT( i >= 0 && i < size());
	int val = checked_cast<int>(i);
    return static_cast<value_type*>(_array.get_ptr())[val]; 
  }
  
  const value_type& array(DomainInt i) const
  { 
    D_ASSERT( i >= 0 && i < size());
	int val = checked_cast<int>(i);
    return static_cast<const value_type*>(_array.get_ptr())[val]; 
  }

  bool needs_undoing()
  {
    D_ASSERT( _local_depth < _max_depth && _local_depth >= _backtrack_depth);

    return _local_depth > _backtrack_depth;
  }

  void undo()
  {
    int bt_depth = _backtrack_depth;

#ifdef DEBUG_TMS
    cout << "About to undo: " ; print_state(); 
#endif
    D_ASSERT( _local_depth < (_max_depth+1) && _local_depth >= bt_depth && bt_depth >=0);

    for(; _local_depth > bt_depth; ) 
    {
      -- _local_depth;
      array(undo_indexes(_local_depth)) = tms_in_set ;
    }

#ifdef DEBUG_TMS
    cout << "Just undone: " ; print_state(); 
#endif

    D_ASSERT(_local_depth == bt_depth);
  }

  bool ifMember_remove(DomainInt index)
  {

         D_ASSERT( 0 <= index && index < size());
    	  if (array(index)) 
	  { 
		  undo_indexes(_local_depth) = checked_cast<int>(index);
		  ++_local_depth;
		 array(index) = 0;	  
		 return 1;
	  }
	  return 0;
  }
  
    bool isMember(DomainInt index) const
  {
    return (bool)array(index);
  }

  void unchecked_remove(DomainInt index)
  {
    D_ASSERT( 0 <= index && index < size());
    undo_indexes(_local_depth) = checked_cast<int>(index);
    ++_local_depth;
    array(index) = 0;
  }
  
  
  int size() const
  {
    return _size;    
  }

void before_branch_left()
  { _backtrack_depth = _local_depth;
    return ; }
    
 void after_branch_left()  // nothing to do
  { return ; }
  
void  before_branch_right()  // nothing to do
  { return ; }
void after_branch_right()  // nothing to do
  { return ; }

void initialise(const int& new_size, const int& max_undos)
  { 
    _size = new_size;
    _max_undos = max_undos;

    // should put in a D_ASSERT on MAXINT here
    // D_ASSERT( max_undos < MAXINT - size);
    
    _max_depth = max_undos;             
    _local_depth = 0;
    _backtrack_depth = 0;

    _array = getMemory(stateObj).nonBackTrack().request_bytes(_size*sizeof(value_type)); 
    _undo_indexes = getMemory(stateObj).nonBackTrack().request_bytes(_max_depth*sizeof(int));

#ifdef DEBUG_TMS
    cout << "initialising TrailedMonotonicSet with value of size= " << size << endl;
    // print_state();
#endif
    
    for(int i=0; i< new_size; i++) {
      array(i) = tms_in_set;
    };
  }

  
  
  TrailedMonotonicSet(StateObj* _stateObj) : stateObj(_stateObj), _backtrack_depth(_stateObj)
  { } 

void print_state()
{
  cout << "printing state of TrailedMonotonicSet: " ;
  cout << "array size: " << _size;
  cout << " local depth: " << _local_depth;
  cout << " backtracking depth: " << _backtrack_depth ; 
  cout << " max depth: " << _max_depth; 
  cout << endl << "   values: " ;
  for(int i = 0; i < _size; ++i) 
  { 
    cout << array(i) << " ";
  }
  cout << endl;
  cout << "  history: ";
  for(int i = 0; i < _local_depth ; ++i) 
  { 
    cout << "[" << undo_indexes(i) //<< ":" << undo_values(i) 
         << "] " ;
  }
  cout << endl ;
}


};

#endif