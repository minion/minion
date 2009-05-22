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


    // This should no longer be used. Use request_storage and lock instead.
/*inline void TrailedMonotonicSet::initialise(const int& new_size, const int& max_undos, StateObj * _stateObj)
{ 
    stateObj=_stateObj;
    
    _size = new_size;
    _max_undos = max_undos;
    
    // should put in a D_ASSERT on MAXINT here
    // D_ASSERT( max_undos < MAXINT - size);
    
    _max_depth = max_undos;             
    _local_depth = 0;
    
    _array = getMemory(stateObj).nonBackTrack().request_bytes(_size*sizeof(value_type)); 
    _undo_indexes = getMemory(stateObj).nonBackTrack().request_bytes(_max_depth*sizeof(int));
    
    _backtrack_depth_ptr = getMemory(stateObj).backTrack().request_bytes(sizeof(int));
    *((int*)_backtrack_depth_ptr.get_ptr()) = 0;
    
    #ifdef DEBUG_TMS
    cout << "initialising TrailedMonotonicSet with value of size= " << size << endl;
    // print_state();
    #endif
    
    for(int i=0; i< new_size; i++) {
      array(i) = tms_in_set;
    };
}*/

  // Must be run before the lock for nonbacktrack.
inline void TrailedMonotonicSet::lock(StateObj * _stateObj)
{
    D_ASSERT(!locked);
    #ifndef NO_DEBUG
    locked=true;
    #endif
    
    stateObj=_stateObj;
    
    _max_undos=_size;
    
    _max_depth = _max_undos;             
    _local_depth = 0;
    
    _array = getMemory(stateObj).nonBackTrack().request_bytes(_size*sizeof(value_type)); 
    _undo_indexes = getMemory(stateObj).nonBackTrack().request_bytes(_max_depth*sizeof(int));
    
    _backtrack_depth_ptr = getMemory(stateObj).backTrack().request_bytes(sizeof(int));
    
    *((int*)_backtrack_depth_ptr.get_ptr()) = 0;
    
    #ifdef DEBUG_TMS
    cout << "initialising TrailedMonotonicSet with value of size= " << size << endl;
    // print_state();
    #endif
    
    for(int i=0; i< _size; i++) {
      array(i) = tms_in_set;
    };
}

