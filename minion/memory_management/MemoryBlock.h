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

/// Looks after all \ref void* to a block of memory, and also the memory itself.
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
  
  size_t allocated_bytes;
  size_t maximum_bytes;
  
  vector<pair<char*, size_t> > stored_blocks;
  size_t total_stored_bytes;

#ifndef BLOCK_SIZE
#define BLOCK_SIZE (size_t)(64*1024*1024)
#endif
  
  SET_TYPE<void**> pointers;
public:
  
  void storeMem(char* store_ptr)
  {
    P("StoreMem: " << (void*)this << " : " << (void*)store_ptr);
    UnsignedSysInt current_offset = 0;
    for(SysInt i = 0; i < stored_blocks.size(); ++i)
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
      // to avoid UnsignedSysInt underflow.
      if(copy_start <= data.second)
          data_copy = std::min(data.second - copy_start, copy_length);
      
      memcpy(location, data.first + copy_start, data_copy);
      memset(location + data_copy, 0, copy_length - data_copy);
  }
public:
    
  void retrieveMem(pair<char*,size_t> store_ptr)
  {
    P("RetrieveMem: " << (void*)this << " : " << (void*)store_ptr);
    UnsignedSysInt current_offset = 0;
    for(SysInt i = 0; i < stored_blocks.size(); ++i)
    {
      copyMemBlock(stored_blocks[i].first, store_ptr, current_offset, stored_blocks[i].second);
      current_offset += stored_blocks[i].second;
    }
    copyMemBlock(current_data, store_ptr, current_offset, allocated_bytes);
    D_ASSERT(getDataSize() == current_offset + allocated_bytes);
  }
  
  /// Returns the size of the allocated memory in bytes.
  UnsignedSysInt getDataSize()
    { return total_stored_bytes + allocated_bytes; }

  NewMemoryBlock() : current_data(NULL), allocated_bytes(0), maximum_bytes(0),
                  total_stored_bytes(0)
  {  }
  
  ~NewMemoryBlock()
  { 
    free(current_data);
  }
  
  /// Request a new block of memory and returns a \ref void* to it's start.
  void* request_bytes(DomainInt byte_count)
  {
    P("Request: " << (void*)this << " : " << byte_count);
    if(byte_count == 0)
      return NULL;
      
    // TODO: is the following line necessary?
    if(byte_count % sizeof(SysInt) != 0)
      byte_count += sizeof(SysInt) - (byte_count % sizeof(SysInt));

    if(maximum_bytes < allocated_bytes + byte_count)
    { reallocate(byte_count); }

    D_ASSERT(maximum_bytes >= allocated_bytes + byte_count);
    char* return_val = current_data + checked_cast<SysInt>(allocated_bytes);
    P("Return val:" << (void*)current_data);
    allocated_bytes += checked_cast<size_t>(byte_count);
    return (void*)return_val;
  }

  /// Request a \ref MoveableArray.
  template<typename T>
  T* requestArray(DomainInt size)
  {
    return (T*)request_bytes(size * sizeof(T));
  }

private:
  void reallocate(DomainInt byte_count_new_request)
  {
    P("Reallocate: " << (void*)this << " : " << byte_count_new_request);
    D_ASSERT(allocated_bytes + byte_count_new_request > maximum_bytes);

    stored_blocks.push_back(make_pair(current_data, allocated_bytes));
    P((void*)current_data << ":" << allocated_bytes << " of " << maximum_bytes);
    total_stored_bytes += allocated_bytes;

    size_t new_block_size = max(BLOCK_SIZE, checked_cast<size_t>(byte_count_new_request));
    current_data = (char*)calloc(new_block_size, sizeof(char));
    if(current_data == NULL)
    { D_FATAL_ERROR("calloc failed - Memory exhausted! Aborting."); }
    P((void*)current_data << " " << new_block_size);
    maximum_bytes = new_block_size;
    allocated_bytes = 0; 
  }
   
};

// @}




#endif
