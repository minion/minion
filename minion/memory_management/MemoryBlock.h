/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: nonbacktrack_memory.h 680 2007-10-01 08:19:53Z azumanga $
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

#ifndef _MEMORYBLOCK_H
#define _MEMORYBLOCK_H

#include "../system/system.h"

/** \defgroup Memory
 *  This group of classes deals with all of Minion's memory management.
 */

// \addtogroup Memory
// @{

/** \brief Class which represents a moveable pointer.
 *
 * This class represents any point in Minion which should be moveable. The
 * two main sources of these are both backtrackable and nonbacktrackable memory.
 * There are always design tradeoffs with this kind of pointer. In this case the
 * choice made has been to make storing and dereferencing the pointer as cheap
 * as possible, while making copying the pointer quite expensive.
 *
 * Note that while this returns a raw pointer, it is important to not keep
 * that pointer across any function call which could either allocate new memory
 * or push or pop the search state stack.
 *
 *
 */
class MoveablePointer
{
  /// The physical pointer being wrapped.
  void* ptr;
public:
  /// Copy constructor for MoveablePointer.
  MoveablePointer(const MoveablePointer& b);
  
  /// Assignment for MoveablePointer.
  void operator=(const MoveablePointer& b);

  /// Constructs a MoveablePointer from a raw pointer ptr.
  explicit MoveablePointer(void* ptr);

  /// Constructs a MoveablePointer from a MoveablePointer and an offset in bytes.
  explicit MoveablePointer(const MoveablePointer& mp, int offset);

  /// In debug mode, gets the pointer without checking if it is valid.
  /** This can be used in non-debug mode, but does not do anything. It is used
   *  by the code which does checking of get_ptr in debug mode.
   */
  void* get_ptr_noCheck() const
  { return ptr; }

  /// Returns the pointer.
  void* get_ptr() const
#ifdef NO_DEBUG
  { return ptr; }
#else
    ; // Defined at bottom of file.
#endif

  /// Manually alters the pointer being watched. This function should only
  /// be used by those with a complete understanding of the whole memory system,
  /// and in special circumstances.
  void set_raw_ptr(void* _ptr) 
  { 
    D_INFO(0, DI_POINTER, "Manually set pointer at " + to_string(this) + " to:" + to_string(ptr));
    ptr = _ptr; 
  }
  
  /// Produces a new MoveablePointer offset from the current one by a given number of bytes.
  MoveablePointer getOffset(unsigned bytes)
  { 
    D_INFO(0, DI_POINTER, "Offset " + to_string(ptr) + " by " + to_string(bytes));
    return MoveablePointer(((char*)ptr) + bytes); 
  }

  /// Default constructor.
  MoveablePointer() : ptr(NULL)
  { D_INFO(0, DI_POINTER, "Create Empty Pointer at:" + to_string(this)); }
  
  /// Destructor.
  ~MoveablePointer();
};

/// Provides a wrapper around \ref MoveablePointer which makes an array.
template<typename T>
class MoveableArray
{
  /// Pointer to start of array.
  MoveablePointer ptr;
  /// Size of array.
  unsigned size;
  
public:
  /// Main constructor, takes a MoveablePointer and the size of the array.
  /** While this can be called manually, it would normally be called by allocateArray
   */
  explicit MoveableArray(MoveablePointer _ptr, unsigned _size) : ptr(_ptr), size(_size)
  { }

  MoveableArray()
  {}

  // A common C++ requiement - declaring two identical methods, one for const, one without.
  
  T& operator[](int pos)
  { 
    D_ASSERT(pos >= 0 && pos < size);
    return *(static_cast<T*>(ptr.get_ptr()) + pos);
  }

  const T& operator[](int pos) const
  { 
    D_ASSERT(pos >= 0 && pos < size);
    return *(static_cast<T*>(ptr.get_ptr()) + pos);
  }

  /// Gets a raw pointer to the start of the array.
  T* get_ptr()
  { return static_cast<T*>(ptr.get_ptr()); }

  /// Gets a const raw pointer to the start of the array.
  const T* get_ptr() const
  { return static_cast<const T*>(ptr.get_ptr()); }
};

typedef MoveablePointer BackTrackOffset;

class NewMemoryBlock;

/// Singleton type which tracks all occurrences of \ref NewMemoryBlock.
/** This class is the only singleton global variable in Minion when in reenterant
 *  mode. It keeps track of all the occurrences of \ref NewMemoryBlock so when a
 *  \ref MoveablePointer is copied, it is possible to find which block it was from.
 */
class MemBlockCache
{
  // Forbid copying this type!
  MemBlockCache(const MemBlockCache&);
  void operator=(const MemBlockCache&);
  vector<NewMemoryBlock*> NewMemoryBlockCache;
  
public:    
  
  MemBlockCache() { }
  


  void registerNewMemoryBlock(NewMemoryBlock* mb)
  { 
    D_ASSERT(find(NewMemoryBlockCache.begin(), NewMemoryBlockCache.end(), mb) == NewMemoryBlockCache.end());
    NewMemoryBlockCache.push_back(mb); 
  }

  void unregisterNewMemoryBlock(NewMemoryBlock* mb)
  { 
    vector<NewMemoryBlock*>::iterator it = find(NewMemoryBlockCache.begin(), NewMemoryBlockCache.end(), mb);
    D_ASSERT(it != NewMemoryBlockCache.end());
    NewMemoryBlockCache.erase(it); 
  }

  inline void addPointerToNewMemoryBlock(MoveablePointer* vp);

  inline void removePointerFromNewMemoryBlock(MoveablePointer* vp);

  inline bool checkPointerValid(const MoveablePointer*const vp);
};

VARDEF(MemBlockCache memBlockCache);


/// Looks after all \ref MoveablePointer to a block of memory, and also the memory itself.
/** A NewMemoryBlock is basically an extendable, moveable block of memory which
 * keeps track of all pointers into it, and moves them when approriate.
 *
 * The main limitation of NewMemoryBlocks is that it is impossible to delete
 * or alter particular allocations without deleting the whole class. This may
 * be fixed in future if it is required.
 */
class NewMemoryBlock
{
  /// Forbid copying.
  NewMemoryBlock(const NewMemoryBlock&);
  /// Forbid copying.
  void operator=(const NewMemoryBlock&);
  
  char* current_data;
  unsigned allocated_bytes;
  unsigned maximum_bytes;
  bool lock_m;
  bool final_lock_m;
  SET_TYPE<MoveablePointer*> pointers;
public:
  
  /// Gets a raw pointer to the start of the allocated memory.
  char* getDataPtr()
  { return current_data; }

  /// Returns the size of the allocated memory in bytes.
  unsigned getDataSize()
  { return allocated_bytes; }

  NewMemoryBlock() : allocated_bytes(0), maximum_bytes(0), current_data(NULL), 
                  lock_m(false), final_lock_m(false)
  { memBlockCache.registerNewMemoryBlock(this);}
  
  ~NewMemoryBlock()
  { 
    memBlockCache.unregisterNewMemoryBlock(this);
    free(current_data);
  }
  
  /// Request a new block of memory and returns a \ref MoveablePointer to it's start.
  MoveablePointer request_bytes(unsigned byte_count)
  {
    // TODO: is the following line necessary?
    if(byte_count % sizeof(int) != 0)
      byte_count += sizeof(int) - (byte_count % sizeof(int));

    D_ASSERT(!lock_m);
    if(maximum_bytes < allocated_bytes + byte_count)
    { reallocate( (allocated_bytes + byte_count) * 2 ); }
    D_INFO(0,DI_MEMBLOCK,"New pointer at:" + to_string((void*)(current_data+allocated_bytes)) + " for " + to_string(byte_count));
    // If no bytes asked for, just stuff it at the start of the block.
    if(byte_count == 0)
      return MoveablePointer(NULL);

    char* return_val = current_data + allocated_bytes;
    allocated_bytes += byte_count;
    return MoveablePointer(return_val);
  }

  /// Request a \ref MoveableArray.
  template<typename T>
  MoveableArray<T> requestArray(unsigned size)
  {
    MoveablePointer ptr = request_bytes(size * sizeof(T));
    return MoveableArray<T>(ptr, size);
  }


  /// Enlarges (or reduces) memory block and moves all \ref MoveablePointer to point to the new block.
  void reallocate(unsigned byte_count)
  {
    D_ASSERT(!lock_m);
    D_ASSERT(byte_count >= allocated_bytes);
    char* new_data = (char*)malloc(byte_count);
    memset(new_data, 0, byte_count);

    for(SET_TYPE<MoveablePointer*>::iterator it = pointers.begin(); it != pointers.end(); ++it)
    {
      D_ASSERT((*it)->get_ptr() >= current_data && (*it)->get_ptr() < current_data + allocated_bytes);
      (*it)->set_raw_ptr( ((char*)((*it)->get_ptr()) - current_data) + new_data );
    }
   
    // TODO: Search codebase for memcpy, use realloc instead if possible.
    memcpy(new_data, current_data, allocated_bytes);
    free(current_data);
    D_INFO(0, DI_MEMBLOCK, "Moving base from " + to_string((void*)current_data) + " to " + to_string((void*)new_data));
    current_data = new_data;
    maximum_bytes = byte_count;
  }

  /// Checks if vp points inside this memoryblock and if so registers it and returns true.
  bool checkAddToTracker(MoveablePointer* vp)
  {
    D_ASSERT(!final_lock_m);
    void* ptr = vp->get_ptr_noCheck();
    D_INFO(0,DI_MEMBLOCK,"Testing if " + to_string(vp) + " to " + to_string(ptr) + " should be added...");
    if(ptr < current_data || ptr >= current_data + allocated_bytes)
      return false;
    else
    {
      D_INFO(0,DI_MEMBLOCK, "Pointer added!");
      pointers.insert(vp);
      return true;
    }
  }

  /// Checks if this pointer belongs to this tracker, and if so unregisters and returns true
  bool checkRemoveFromTracker(MoveablePointer* vp)
  {
    void* ptr = vp->get_ptr_noCheck();
    if(ptr < current_data || ptr >= current_data + allocated_bytes)
      return false;
    else
    {
      D_INFO(0,DI_MEMBLOCK, "Pointer removed!");
      pointers.erase(vp);
      return true;
    }
  }

  /// Checks if a given pointer should point into this block and if so checks for consistency
  /* Used only in debug mode for validation. Should never fail. */
  bool checkPointerValid(const MoveablePointer*const vp)
  {
    void* ptr = vp->get_ptr_noCheck();
    // This message is useful when debugging memory issues, but prints far too much in normal usage.
    // D_INFO(0, DI_MEMBLOCK, "Checking for " + to_string(vp) + " to " + to_string(ptr) + " in range " + to_string((void*) current_data) + string(":")
    //  + to_string((void*)(current_data + allocated_bytes)));

    bool check1 = (pointers.count(const_cast<MoveablePointer*>(vp)) > 0);
    bool check2 = (ptr >= current_data && ptr < current_data + allocated_bytes);
    if(check1 != check2)
    {
      D_INFO(0, DI_MEMBLOCK, "Error!" + to_string(check1) + ":" + to_string(check2));
      D_FATAL_ERROR("Fatal Memory corruption - pointer broken!");
    }
    return check1;
  }

  // TODO: Remove
  void final_lock()
  { 
    D_ASSERT(lock_m);
    final_lock_m = true;
  }
  
  void lock()
  {
    reallocate(allocated_bytes);
    lock_m = true;
  }
   
};

// @}

inline MoveablePointer::MoveablePointer(const MoveablePointer& b) : ptr(b.ptr)
{
  D_INFO(0,DI_POINTER,"Copy pointer to " + to_string(ptr) + " from vp " + to_string(&b) + " to " + to_string(this));
  if(ptr != NULL)
    memBlockCache.addPointerToNewMemoryBlock(this);
}

inline void MoveablePointer::operator=(const MoveablePointer& b)
{
  D_INFO(0, DI_POINTER, "Make " + to_string(this)+ " point to " + to_string(b.ptr) + " instead of " + to_string(ptr));
  if(ptr != NULL)
    memBlockCache.removePointerFromNewMemoryBlock(this);
  ptr = b.ptr;
  if(ptr != NULL)
    memBlockCache.addPointerToNewMemoryBlock(this);
}

inline MoveablePointer::MoveablePointer(void* _ptr) : ptr(_ptr)
{
  D_INFO(0,DI_POINTER,"Create pointer from raw to:" + to_string(ptr) + " at " + to_string(this));
  if(ptr != NULL)
    memBlockCache.addPointerToNewMemoryBlock(this);
}

inline MoveablePointer::~MoveablePointer()
{
  D_INFO(0,DI_POINTER,"Remove pointer at " + to_string(this));
  if(ptr != NULL)
    memBlockCache.removePointerFromNewMemoryBlock(this);
}

inline MoveablePointer::MoveablePointer(const MoveablePointer& b, int offset) : ptr(((char*)b.ptr) + offset)
{
  D_INFO(0,DI_POINTER,"Create offset of " + to_string(b.ptr) + " by " + to_string(offset));
  D_ASSERT(b.get_ptr() != NULL);
  memBlockCache.addPointerToNewMemoryBlock(this);
}


#ifndef NO_DEBUG
inline void* MoveablePointer::get_ptr() const
{
  D_ASSERT(memBlockCache.checkPointerValid(this));
  return ptr;
}
#endif

inline void MemBlockCache::addPointerToNewMemoryBlock(MoveablePointer* vp)
  {
    if(vp->get_ptr_noCheck() == NULL)
      return;

    for(vector<NewMemoryBlock*>::iterator it = NewMemoryBlockCache.begin();
        it != NewMemoryBlockCache.end();
        ++it)
    {
      if((*it)->checkAddToTracker(vp))
        return;
    }
    D_FATAL_ERROR("Fatal Memory Corruption when adding to tracker!");
  }

  inline void MemBlockCache::removePointerFromNewMemoryBlock(MoveablePointer* vp)
  {
    for(vector<NewMemoryBlock*>::iterator it = NewMemoryBlockCache.begin();
        it != NewMemoryBlockCache.end();
        ++it)
    {
      if((*it)->checkRemoveFromTracker(vp))
        return;
    }
    //D_FATAL_ERROR("Fatal Memory Corruption when leaving the tracker");
  }

  inline bool MemBlockCache::checkPointerValid(const MoveablePointer *const vp)
  {
    if(vp->get_ptr_noCheck() == NULL)
      return true;
    for(vector<NewMemoryBlock*>::iterator it = NewMemoryBlockCache.begin();
        it != NewMemoryBlockCache.end();
        ++it)
    {
      if((*it)->checkPointerValid(vp))
        return true;
    }
    ;
    D_FATAL_ERROR("Fatal Memory Error - invalid Pointer deferenced!");
  }

#endif
