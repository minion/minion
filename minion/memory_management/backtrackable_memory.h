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

/**
 * This file deals with allocating, deallocating and maintaining backtrackable
 * memory.
 *
 * The main reason this is non-trivial is that we don't know at first how much
 * memory we will need. Therefore, work progresses in 2 phases.
 *
 * 1) All the memory wanted is created. During this phase, all pointers to 
 * backtrackable memory are kept in a central register.
 *
 * 2) All backtrackable memory is moved into one place. At this point all 
 * memory is merged into one block.
 *
 * There are two main classes of interest. BackTrackOffsets represent an offset
 * into the backtrackable memory. They are created with a call to 
 * backtrackable_memory.get_bytes.
 *
 * VirtualBackTrackOffsets represent an offset into an existing block of 
 * backtrackable memory. They are created from a BackTrackOffset and a position.
 *
 * Note that both BackTrackOffset and VirtualBackTrackOffset have been designed
 * so that they are as efficent as just using a plain pointer during search 
 * itself.
 */

#ifdef VM_COPY
#define SEPERATE_PAGES
#include <mach/mach.h>
#endif

#ifdef NO_DOUBLE_MEMORY_COPY
VARDEF_ASSIGN(void* memory_base_ptr, NULL);
#endif

extern TableOut tableout;

/// A pointer to some backtrackable memory.
class BackTrackOffset
{
  void* ptr;
public:
  BackTrackOffset(const BackTrackOffset& b);

  void set_ptr(void* new_ptr)
  { 
	D_ASSERT((size_t)(new_ptr) % sizeof(int) == 0);
	ptr = new_ptr;
  }
  
  void* get_ptr() const 
  { 
	D_ASSERT((size_t)(ptr) % sizeof(int) == 0);
#ifdef NO_DOUBLE_MEMORY_COPY
	return (char*)memory_base_ptr + (int)ptr;
#else
	return ptr;
#endif
  }
  void request_bytes(int i);
  BackTrackOffset();
  ~BackTrackOffset();  
};

/// A pointer into an existing block of backtrackable memory.
struct VirtualBackTrackOffset
{
  void* ptr;
  void* get_ptr() const 
  {
#ifdef NO_DOUBLE_MEMORY_COPY
	return (char*)memory_base_ptr + (int)ptr;
#else
	return ptr; 
#endif
  }
  VirtualBackTrackOffset();
  VirtualBackTrackOffset(BackTrackOffset& b, int offset);
  VirtualBackTrackOffset(const VirtualBackTrackOffset&);
  void operator=(const VirtualBackTrackOffset&);
  ~VirtualBackTrackOffset();  
};

/// Container for setting up and maintaining backtrackable memory.
struct BacktrackableMemory
{
  char* current_data;
#ifdef SEPERATE_PAGES
  char** backtrack_cache;
#else
  char* backtrack_cache;
#endif
  unsigned backtrack_cache_size;
  unsigned backtrack_cache_offset;
  unsigned allocated_bytes;
  BOOL lock_m;
  BOOL final_lock_m;
  set<BackTrackOffset*> offsets;
  MAP_TYPE<void*, pair<int,int> > offset_positions;
  MAP_TYPE<VirtualBackTrackOffset*, pair<BackTrackOffset*, int> > virtual_ptrs;
  
  BacktrackableMemory() : allocated_bytes(0), lock_m(false), final_lock_m(false)
  { }
  
  /// Get a block of backtrack memory
  BackTrackOffset get_bytes(unsigned byte_count)
  { 
	// XXX for now, will pad this up. If people ask for lots of tiny blocks,
	// this could be bad.
	if(byte_count % sizeof(int) != 0)
	  byte_count += sizeof(int) - (byte_count % sizeof(int));
	D_ASSERT(byte_count % sizeof(int) == 0);
    D_ASSERT(!lock_m);
    BackTrackOffset new_mem;
    // :(
    char* ptr = new char[byte_count];
	D_ASSERT((size_t) ptr % sizeof(int) == 0);
    std::fill(ptr, ptr + byte_count, 0);
    offset_positions[ptr] = make_pair(allocated_bytes, byte_count);
    new_mem.set_ptr(ptr);
    allocated_bytes += byte_count; 
    return new_mem;
  }

  /// Add a reference to a backtrackable memory block.
  /// Is done in BackTrackOffset's constructor.
  void addToTracker(BackTrackOffset* bto)
  {
    D_ASSERT(!lock_m);
    D_ASSERT(offsets.count(bto) == 0);
    offsets.insert(bto);
  }

  /// Remove a reference to a backtrackable memory block.
  /// Is done in BackTrackOffset's destructor.
  void removeFromTracker(BackTrackOffset* bto)
  {
    if(Controller::finished)
      return;
    D_ASSERT(!lock_m);
    //D_DATA(char* old_ptr = static_cast<char*>(bto->ptr));
    //D_ASSERT(offset_positions.count(old_ptr) == 0);
    offsets.erase(bto);
  }

  /// Add a reference to a virtual backtrackable pointer based on an existing one.
  /// Called in VirtualBackTrackOffset's copy constructor.
  void addToVirtualTracker(VirtualBackTrackOffset* bto, const VirtualBackTrackOffset* original)
  {
    if(!final_lock_m)
	{
      D_ASSERT(virtual_ptrs.count(bto) == 0);
      if(!lock_m)
      {
        if(virtual_ptrs.count(const_cast<VirtualBackTrackOffset*>(original)))
          virtual_ptrs[bto] = virtual_ptrs[const_cast<VirtualBackTrackOffset*>(original)];
      }
	}
  }
  
  /// Add a reference to a virtual backtrackable pointer based on a backtrackoffset and a position into that memory's block.
  /// Is handled in VirtualBackTrackOffset's constructor.
  void addToVirtualTracker(VirtualBackTrackOffset* bto, BackTrackOffset* original, int offset)
  {
    // if final_lock_m has occured, the point is (we hope) already in the right place.
	// There is no moving to be done.
    if(!final_lock_m)
	{
      D_ASSERT(virtual_ptrs.count(bto) == 0);
      virtual_ptrs[bto] = make_pair(original, offset);
	}
  }
  
  /// Remove a virtual backtrackable pointer from the storage.
  /// Handled in VirtualBackTrackOffset's destructor.
  void removeFromVirtualTracker(VirtualBackTrackOffset* bto)
  {
    if(!final_lock_m)
	{ virtual_ptrs.erase(bto); }
  }
  
  
  void final_lock()
  { 
    final_lock_m = true;
    offset_positions.clear();
    offsets.clear();
    virtual_ptrs.clear();
    cout << "Bytes used in Backtrackable Memory = " << allocated_bytes << endl;
    tableout.set("BacktrackableMemoryBytes", allocated_bytes);
  }
  
  void lock()
  {
    D_ASSERT(!lock_m);
    D_ASSERT(!final_lock_m);
    lock_m = true;
#ifdef VM_COPY
	allocated_bytes + ((vm_page_size - (allocated_bytes % vm_page_size)) % vm_page_size);
#endif
    current_data = new char[allocated_bytes];
	
	D_ASSERT( (size_t)current_data % sizeof(int) == 0);
    MAP_TYPE<void*, pair<int,int> > offset_positions_backup(offset_positions);
    // Code like this makes baby Jesus cry, but is suprisingly legal C++
    // The apparently equivalent "offsets[i]->ptr = (int)offset" isn't.
    for(set<BackTrackOffset*>::iterator it=offsets.begin();it!=offsets.end();++it)
    {
      char* old_ptr = static_cast<char*>((*it)->get_ptr());
	  if(old_ptr != NULL)
	  {
        D_ASSERT(offset_positions.count(old_ptr) == 1);

#ifdef NO_DOUBLE_MEMORY_COPY
		(*it)->set_ptr(((char*)0) + offset_positions[old_ptr].first);
#else
		(*it)->set_ptr(current_data + offset_positions[old_ptr].first);
#endif
		
		D_ASSERT( (size_t)((*it)->get_ptr()) % sizeof(int) == 0);
        
		copy(old_ptr, old_ptr + offset_positions[old_ptr].second,
	     current_data + offset_positions[old_ptr].first);
        
		delete[] old_ptr;
	  }
      D_DATA(offset_positions.erase(old_ptr));
    }
    D_ASSERT(offset_positions.size() == 0);
    
    for(MAP_TYPE<VirtualBackTrackOffset*, pair<BackTrackOffset*, int> >::iterator it = virtual_ptrs.begin(); 
	it != virtual_ptrs.end(); ++it)
    {
      VirtualBackTrackOffset* ptr = it->first;
      const BackTrackOffset* master = it->second.first;
      int offset_val = it->second.second;

      ptr->ptr = static_cast<char*>(master->get_ptr()) + offset_val;
    }
#ifdef NO_DOUBLE_MEMORY_COPY
	memory_base_ptr = current_data;
#endif

#ifdef SEPERATE_PAGES
	backtrack_cache = (char**)malloc(100 * sizeof(char*));
	for(int i = 0; i < 100; ++i)
	  backtrack_cache[i] = (char*)malloc(allocated_bytes);
#else
    backtrack_cache = new char[allocated_bytes * 100];
#endif
	
    backtrack_cache_size = allocated_bytes * 100;
    backtrack_cache_offset = 0;
  }
  
  void world_push()
  {
    D_ASSERT(lock_m);
    
	if(backtrack_cache_offset == backtrack_cache_size)
    {
#ifdef SEPERATE_PAGES
	  int pages = backtrack_cache_size / allocated_bytes;
	  backtrack_cache = (char**)realloc(backtrack_cache, pages * 2 * sizeof(char**));
	  for(int i = pages; i < pages * 2; ++i)
		backtrack_cache[i] = (char*)malloc(allocated_bytes);
#else
      char* new_backtrack_cache = new char[backtrack_cache_size * 2];
      memcpy(new_backtrack_cache, backtrack_cache, backtrack_cache_size);
      delete[] backtrack_cache;
      backtrack_cache = new_backtrack_cache;
#endif
	  backtrack_cache_size *= 2;
    }
    
#ifdef SEPERATE_PAGES
	
#ifdef VM_COPY
	vm_copy(mach_task_self(), (vm_address_t)backtrack_cache[backtrack_cache_offset / allocated_bytes], allocated_bytes, (vm_address_t)current_data);
#else
	memcpy(backtrack_cache[backtrack_cache_offset / allocated_bytes], current_data, allocated_bytes);
#endif
	
#else
	char* old_world = backtrack_cache + backtrack_cache_offset;
    memcpy(old_world, current_data, allocated_bytes);
#endif

    backtrack_cache_offset += allocated_bytes;
  }
  
  void world_pop()
  {
    D_ASSERT(lock_m);
    D_ASSERT(backtrack_cache_offset >= allocated_bytes);
    backtrack_cache_offset -= allocated_bytes;
#ifdef SEPERATE_PAGES
	
#ifdef VM_COPY
	vm_copy(mach_task_self(), (vm_address_t)current_data, allocated_bytes, (vm_address_t)backtrack_cache[backtrack_cache_offset / allocated_bytes]);
#else
    memcpy(current_data, backtrack_cache[backtrack_cache_offset / allocated_bytes], allocated_bytes);
#endif

#else
    memcpy(current_data, backtrack_cache + backtrack_cache_offset, allocated_bytes);
#endif
  }
  
  int current_depth()
  { 
	D_ASSERT(backtrack_cache_offset % allocated_bytes == 0);
	return backtrack_cache_offset / allocated_bytes;
  }
};

VARDEF(BacktrackableMemory backtrackable_memory);

inline  BackTrackOffset::BackTrackOffset() : ptr(NULL)
{backtrackable_memory.addToTracker(this);}

inline VirtualBackTrackOffset::VirtualBackTrackOffset() : ptr(NULL)
{ }

inline BackTrackOffset::BackTrackOffset(const BackTrackOffset& b) : ptr(b.ptr)
{backtrackable_memory.addToTracker(this);}

inline void VirtualBackTrackOffset::operator=(const VirtualBackTrackOffset& b)
{
  backtrackable_memory.removeFromVirtualTracker(this);
  backtrackable_memory.addToVirtualTracker(this, &b); 
}

inline VirtualBackTrackOffset::VirtualBackTrackOffset(const VirtualBackTrackOffset& b) : ptr(b.ptr)
{  backtrackable_memory.addToVirtualTracker(this, &b); }

inline VirtualBackTrackOffset::VirtualBackTrackOffset(BackTrackOffset& b, int offset) : ptr(b.get_ptr())
{ backtrackable_memory.addToVirtualTracker(this, &b, offset); }

inline  BackTrackOffset::~BackTrackOffset()
{ backtrackable_memory.removeFromTracker(this); }
  
inline VirtualBackTrackOffset::~VirtualBackTrackOffset()
{ backtrackable_memory.removeFromVirtualTracker(this); }

inline void BackTrackOffset::request_bytes(int i)
{ *this = backtrackable_memory.get_bytes(i); }


