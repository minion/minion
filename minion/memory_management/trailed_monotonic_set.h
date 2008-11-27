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
  
  #ifndef NO_DEBUG
  bool locked;
  #endif
  
  int _size;
  int _max_undos;
  int _max_depth;             

  int _local_depth;             // could be unsigned 
  
  MoveablePointer _backtrack_depth_ptr;
  
  MemOffset _array;
  MemOffset _undo_indexes;
  
  //MoveablePointer backtrack_ptr;

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
    D_ASSERT( _local_depth < _max_depth && _local_depth >= (*((int*)_backtrack_depth_ptr.get_ptr())) );

    return _local_depth > (*((int*)_backtrack_depth_ptr.get_ptr()));
  }

  void undo()
  {
    int bt_depth = (*((int*)_backtrack_depth_ptr.get_ptr()));

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
     // cout << "Size:"<< _size << ", index:"<<index<<endl;
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
  { *((int*)_backtrack_depth_ptr.get_ptr()) = _local_depth;
    return ; }
    
 void after_branch_left()  // nothing to do
  { return ; }
  
void  before_branch_right()  // nothing to do
  { return ; }
void after_branch_right()  // nothing to do
  { return ; }

    int request_storage(int allocsize)
    {
        D_ASSERT(!locked);
        // returns the index of the first element allocated.
        int blockindex=_size;
        _size=_size+allocsize;
        return blockindex;
    }
  
    // This should no longer be used. Use request_storage and lock instead.
  //void initialise(const int& new_size, const int& max_undos, StateObj stateObj);

  // Must be run before the lock for nonbacktrack.
  void lock(StateObj * stateObj);
  
  //TrailedMonotonicSet(StateObj* _stateObj) : stateObj(_stateObj) //, _backtrack_depth(_stateObj)
  //{ } 
  
  TrailedMonotonicSet()
  #ifndef NO_DEBUG
  : locked(false),
  #endif
  _size(0), _max_undos(0), _max_depth(0), _local_depth(0)
  { }
  
void print_state()
{
  cout << "printing state of TrailedMonotonicSet: " ;
  cout << "array size: " << _size;
  cout << " local depth: " << _local_depth;
  cout << " backtracking depth: " << *((int*)_backtrack_depth_ptr.get_ptr()) ; 
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

typedef TrailedMonotonicSet MonotonicSet;
#endif

