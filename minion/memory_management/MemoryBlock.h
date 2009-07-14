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

#ifndef _MEMORYBLOCK_H
#define _MEMORYBLOCK_H

#include "../system/system.h"

#ifdef P
#undef P
#endif

#define P(x)
//#define P(x) std::cout << x << std::endl

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
#ifndef SLOW_DEBUG
  { return ptr; }
#else
    ; // Defined at bottom of file.
#endif

  bool is_null() const
  { return ptr == NULL; }

  /// Manually alters the pointer being watched. This function should only
  /// be used by those with a complete understanding of the whole memory system,
  /// and in special circumstances.
  void set_raw_ptr(void* _ptr) 
  { 
    ptr = _ptr; 
  }
  
  /// Produces a new MoveablePointer offset from the current one by a given number of bytes.
  MoveablePointer getOffset(unsigned bytes)
  { 
    return MoveablePointer(((char*)ptr) + bytes); 
  }

  /// Default constructor.
  MoveablePointer() : ptr(NULL)
  { }
  
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
  
  vector<pair<char*, unsigned> > stored_blocks;
  int total_stored_bytes;

#ifndef BLOCK_SIZE
#define BLOCK_SIZE (16*1024*1024)
#endif
  
  SET_TYPE<MoveablePointer*> pointers;
public:
  
  void storeMem(char* store_ptr)
  {
    P("StoreMem: " << (void*)this << " : " << (void*)store_ptr);
    unsigned current_offset = 0;
    for(int i = 0; i < stored_blocks.size(); ++i)
    {
      P((void*)(store_ptr + current_offset) << " " << (void*)stored_blocks[i].first << " " << stored_blocks[i].second);
      memcpy(store_ptr + current_offset, stored_blocks[i].first, stored_blocks[i].second);
      current_offset += stored_blocks[i].second;
    }

    P((void*)(store_ptr + current_offset) << " " << (void*)current_data << " " << allocated_bytes);    
    memcpy(store_ptr + current_offset, current_data, allocated_bytes);
    D_ASSERT(getDataSize() == current_offset + allocated_bytes);
  }

private:
  void copyMemBlock(char* location, pair<char*,size_t> data, size_t copy_start, size_t copy_length)
  {
      D_ASSERT(data.second >= copy_start + copy_length);
      //memcpy(location, data.first + copy_start, copy_length);
      
      size_t data_copy = 0;
      // If these is some data to copy, then we do so. We write the code this way
      // to avoid unsigned underflow.
      if(copy_start <= data.second)
          data_copy = std::min(data.second - copy_start, copy_length);
      
      memcpy(location, data.first + copy_start, data_copy);
      memset(location + data_copy, 0, copy_length - data_copy);
  }
public:
    
  void retrieveMem(pair<char*,size_t> store_ptr)
  {
    P("RetrieveMem: " << (void*)this << " : " << (void*)store_ptr);
    unsigned current_offset = 0;
    for(int i = 0; i < stored_blocks.size(); ++i)
    {
      copyMemBlock(stored_blocks[i].first, store_ptr, current_offset, stored_blocks[i].second);
      current_offset += stored_blocks[i].second;
    }
    copyMemBlock(current_data, store_ptr, current_offset, allocated_bytes);
    D_ASSERT(getDataSize() == current_offset + allocated_bytes);
  }
  
  /// Returns the size of the allocated memory in bytes.
  unsigned getDataSize()
    { return total_stored_bytes + allocated_bytes; }

  NewMemoryBlock() : current_data(NULL), allocated_bytes(0), maximum_bytes(0),
                  total_stored_bytes(0)
  {  }
  
  ~NewMemoryBlock()
  { 
    free(current_data);
  }
  
  /// Request a new block of memory and returns a \ref MoveablePointer to it's start.
  MoveablePointer request_bytes(unsigned byte_count)
  {
    P("Request: " << (void*)this << " : " << byte_count);
    if(byte_count == 0)
      return MoveablePointer(NULL);
      
    // TODO: is the following line necessary?
    if(byte_count % sizeof(int) != 0)
      byte_count += sizeof(int) - (byte_count % sizeof(int));

    if(maximum_bytes < allocated_bytes + byte_count)
    { reallocate(byte_count); }

    D_ASSERT(maximum_bytes >= allocated_bytes + byte_count);
    char* return_val = current_data + allocated_bytes;
    P("Return val:" << (void*)current_data);
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

private:
  void reallocate(unsigned byte_count_new_request)
  {
    P("Reallocate: " << (void*)this << " : " << byte_count_new_request);
    D_ASSERT(allocated_bytes + byte_count_new_request > maximum_bytes);

    stored_blocks.push_back(make_pair(current_data, allocated_bytes));
    P((void*)current_data << ":" << allocated_bytes << " of " << maximum_bytes);
    total_stored_bytes += allocated_bytes;

    unsigned new_block_size = max((unsigned)BLOCK_SIZE, byte_count_new_request);
    current_data = (char*)malloc(new_block_size);
    memset(current_data, 0, new_block_size);
    P((void*)current_data << " " << new_block_size);
    maximum_bytes = new_block_size;
    allocated_bytes = 0; 
  }
   
};

// @}


inline MoveablePointer::MoveablePointer(const MoveablePointer& b) : ptr(b.ptr)
{ }

inline void MoveablePointer::operator=(const MoveablePointer& b)
{ ptr = b.ptr; }

inline MoveablePointer::MoveablePointer(void* _ptr) : ptr(_ptr)
{ }

inline MoveablePointer::~MoveablePointer()
{ }

inline MoveablePointer::MoveablePointer(const MoveablePointer& b, int offset) : ptr(((char*)b.ptr) + offset)
{ D_ASSERT(b.get_ptr() != NULL); }


#ifdef SLOW_DEBUG
inline void* MoveablePointer::get_ptr() const
{ return ptr; }
#endif

#endif
