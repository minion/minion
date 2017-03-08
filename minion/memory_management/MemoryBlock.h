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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
* USA.
*/

#ifndef _MEMORYBLOCK_H
#define _MEMORYBLOCK_H

#include "../system/system.h"
#include "../system/block_cache.h"

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
/** class BackTrackMemory is basically an extendable, moveable block of memory which
 * keeps track of all pointers into it, and moves them when approriate.
 *
 */

// Small wrapper to represent an extendable block
class ExtendableBlock {
  // Pointer to block
  char* ptr;
  // Position in BackTrackMemory of this block
  SysInt pos;
public:
  ExtendableBlock() : ptr(NULL), pos(-1)
  { }

  ExtendableBlock(char* _ptr, SysInt _pos) : ptr(_ptr), pos(_pos)
  { }

  ExtendableBlock(const ExtendableBlock&) = default;

  char* operator()() const
  { return ptr; }

  SysInt getPos() const
  { return pos; }

  bool empty() const
  { return ptr == nullptr; }
};

class BackTrackMemory {
  /// Forbid copying.
  BackTrackMemory(const BackTrackMemory&);
  /// Forbid copying.
  void operator=(const BackTrackMemory&);

  // Variables for backtracking
  BlockCache block_cache;
  vector<pair<char*, UnsignedSysInt>> backtrack_data;

  struct BlockDef
  {
    char* base;
    size_t size;
    size_t capacity;

    BlockDef() : base(0), size(0), capacity(0)
    { }

    BlockDef(char* b, size_t s, size_t c)
    : base(b), size(s), capacity(c)
    { }
  };

  BlockDef current;

  // previous, filled blocks
  vector<BlockDef> stored_blocks;
  // byte inside stored_blocks
  size_t total_stored_bytes;


  vector<BlockDef> extendable_blocks;
  size_t allocated_extendable_bytes;


#ifndef BLOCK_SIZE
#define BLOCK_SIZE (size_t)(64 * 1024 * 1024)
#endif


public:
  void copyIntoPtr(char* store_ptr) {
    P("StoreMem: " << (void*)this << " : " << (void*)store_ptr);
    UnsignedSysInt current_offset = 0;
    for(SysInt i = 0; i < (SysInt)stored_blocks.size(); ++i) {
      P((void*)(store_ptr + current_offset) << " " << (void*)stored_blocks[i].base << " "
                                            << stored_blocks[i].size);
      memcpy(store_ptr + current_offset, stored_blocks[i].base, stored_blocks[i].size);
      current_offset += stored_blocks[i].size;
    }

    P((void*)(store_ptr + current_offset) << " " << (void*)current.base << " " << current.size);
    memcpy(store_ptr + current_offset, current.base, current.size);

    current_offset += current.size;

    for(SysInt i = 0; i < (SysInt)extendable_blocks.size(); ++i) {
      memcpy(store_ptr + current_offset, extendable_blocks[i].base, extendable_blocks[i].size);
      current_offset += extendable_blocks[i].size;
    }

    D_ASSERT(getDataSize() == current_offset);
  }

private:
  void copyMemBlock(char* location, pair<char*, size_t> data, size_t copy_start,
                    size_t copy_length) {
    D_ASSERT(data.second >= copy_start + copy_length);
    // memcpy(location, data.base + copy_start, copy_length);

    size_t data_copy = 0;
    // If these is some data to copy, then we do so. We write the code this way
    // to avoid UnsignedSysInt underflow.
    if(copy_start <= data.second)
      data_copy = std::min(data.second - copy_start, copy_length);

    memcpy(location, data.first + copy_start, data_copy);
    memset(location + data_copy, 0, copy_length - data_copy);
  }

public:
  void retrieveFromPtr(pair<char*, size_t> store_ptr) {
    P("RetrieveMem: " << (void*)this << " : " << (void*)store_ptr);
    UnsignedSysInt current_offset = 0;
    for(SysInt i = 0; i < (SysInt)stored_blocks.size(); ++i) {
      copyMemBlock(stored_blocks[i].base, store_ptr, current_offset, stored_blocks[i].size);
      current_offset += stored_blocks[i].size;
    }
    copyMemBlock(current.base, store_ptr, current_offset, current.size);

    current_offset += current.size;

    for(SysInt i = 0; i < (SysInt)extendable_blocks.size(); ++i) {
      memcpy(extendable_blocks[i].base, store_ptr.first + current_offset, extendable_blocks[i].size);
      current_offset += extendable_blocks[i].size;
    }

    D_ASSERT(getDataSize() == current_offset);
  }

  /// Returns the size of the allocated memory in bytes.
  UnsignedSysInt getDataSize() {
    return total_stored_bytes + current.size + allocated_extendable_bytes;
  }

  BackTrackMemory()
      : block_cache(100), 
        total_stored_bytes(0),
        allocated_extendable_bytes(0)
      { }

  ~BackTrackMemory() {
    free(current.base);

    for(SysInt i = 0; i < (SysInt)backtrack_data.size(); ++i)
      block_cache.do_free(backtrack_data[i].first);
  }

  /// Copies the current state of backtrackable memory.
  void world_push() {
    UnsignedSysInt data_size = this->getDataSize();
    char* tmp = (char*)block_cache.do_malloc(data_size); // calloc(data_size, sizeof(char));

    this->copyIntoPtr(tmp);
    backtrack_data.push_back(std::make_pair(tmp, data_size));
  }

  /// Restores the state of backtrackable memory to the last stored state.
  void world_pop() {
    D_ASSERT(backtrack_data.size() > 0);
    pair<char*, size_t> tmp = backtrack_data.back();
    this->retrieveFromPtr(tmp);
    backtrack_data.pop_back();
    block_cache.do_free(tmp.first);
  }

    /// Returns the current number of stored copies of the state.
  SysInt current_depth() {
    return backtrack_data.size();
  }

  void world_pop_to(SysInt depth) {
    D_ASSERT(current_depth() >= depth);
    while(current_depth() > depth)
      world_pop();
    D_ASSERT(current_depth() == depth);
  }


  /// Request a new block of memory and returns a \ref void* to it's start.
  void* request_bytes(DomainInt byte_count) {
    P("Request: " << (void*)this << " : " << byte_count);
    if(byte_count == 0)
      return NULL;

    // TODO: is the following line necessary?
    if(byte_count % sizeof(SysInt) != 0)
      byte_count += sizeof(SysInt) - (byte_count % sizeof(SysInt));

    if((DomainInt)current.capacity < (DomainInt)(current.size) + byte_count) {
      reallocate(byte_count);
    }

    D_ASSERT((DomainInt)current.capacity >= current.size + byte_count);
    char* return_val = current.base + checked_cast<SysInt>(current.size);
    P("Return val:" << (void*)current.base);
    current.size += checked_cast<size_t>(byte_count);
    return (void*)return_val;
  }

  ExtendableBlock requestBytesExtendable(UnsignedSysInt base_size)
  {
    const SysInt max_size = 10*1024*1024;
    char* block = (char*)calloc(max_size, 1);
    extendable_blocks.push_back(BlockDef{block, base_size, max_size});
    allocated_extendable_bytes += base_size;
    return ExtendableBlock{block, (SysInt)extendable_blocks.size() - 1};
  }

  void resizeExtendableBlock(ExtendableBlock block, UnsignedSysInt new_size)
  {
    UnsignedSysInt old_size = extendable_blocks[block.getPos()].size;
    D_ASSERT(block() == extendable_blocks[block.getPos()].base);
    D_ASSERT(new_size >= old_size);
    D_ASSERT(new_size <= extendable_blocks[block.getPos()].capacity);
    D_ASSERT(checkAllZero(block() + old_size, block() + new_size));

    allocated_extendable_bytes += (new_size - old_size);

    extendable_blocks[block.getPos()].size = new_size;
    //block_resizes.back().push_back(make_tuple(block.getPos(), old_size, new_size));

  }

  /// Request a \ref MoveableArray.
  template <typename T>
  T* requestArray(DomainInt size) {
    return (T*)request_bytes(size * sizeof(T));
  }

private:
  void reallocate(DomainInt byte_count_new_request) {
    P("Reallocate: " << (void*)this << " : " << byte_count_new_request);
    D_ASSERT(current.size + byte_count_new_request > (DomainInt)current.capacity);

    stored_blocks.push_back(current);
    P((void*)current.base << ":" << current.size << " of " << current.capacity);
    total_stored_bytes += current.size;

    size_t new_block_size = max(BLOCK_SIZE, checked_cast<size_t>(byte_count_new_request));
    current.base = (char*)calloc(new_block_size, sizeof(char));
    if(current.base == NULL) {
      D_FATAL_ERROR("calloc failed - Memory exhausted! Aborting.");
    }
    P((void*)current.base << " " << new_block_size);
    current.capacity = new_block_size;
    current.size = 0;
  }
};

// @}

#endif
