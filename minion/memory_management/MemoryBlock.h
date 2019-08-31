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

#include "../system/block_cache.h"
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
  ExtendableBlock() : ptr(NULL), pos(-1) {}

  ExtendableBlock(char* _ptr, SysInt _pos) : ptr(_ptr), pos(_pos) {}

  ExtendableBlock(const ExtendableBlock&) = default;

  char* operator()() const {
    return ptr;
  }

  SysInt getPos() const {
    return pos;
  }

  bool empty() const {
    return ptr == nullptr;
  }
};

class BackTrackMemory {
  /// Forbid copying.
  BackTrackMemory(const BackTrackMemory&);
  /// Forbid copying.
  void operator=(const BackTrackMemory&);

  struct BlockDef {
    char* base;
    size_t size;
    size_t capacity;

    BlockDef() : base(0), size(0), capacity(0) {}

    BlockDef(char* b, size_t s, size_t c) : base(b), size(s), capacity(c) {}

    size_t remaining_capacity() const {
      return capacity - size;
    }
  };

  // previous, filled blocks
  vector<BlockDef> stored_blocks;
  // byte inside stored_blocks
  size_t total_stored_bytes;

  vector<BlockDef> extendable_blocks;
  size_t allocated_extendable_bytes;

  // Store information about backtracking.
  struct BacktrackData {
    size_t total_stored_bytes;
    size_t allocated_extendable_bytes;
    vector<size_t> extendable_blocksSize;

    char* data;
    size_t total_bytes() const {
      return total_stored_bytes + allocated_extendable_bytes;
    }

    BacktrackData() : total_stored_bytes(0), allocated_extendable_bytes(0), data(0) {}

    BacktrackData(size_t t, size_t a, vector<size_t> v, char* d)
        : total_stored_bytes(t),
          allocated_extendable_bytes(a),
          extendable_blocksSize(v),
          data(d) {}
  };

  // Variables for backtracking
  BlockCache block_cache;
  // vector<pair<char*, UnsignedSysInt>> backtrack_data;
  vector<BacktrackData> backtrack_stack;

#ifndef BLOCK_SIZE
#define BLOCK_SIZE (size_t)(64 * 1024 * 1024)
#endif

public:
  void copyIntoPtr(char* storePtr) {
    P("StoreMem: " << (void*)this << " : " << (void*)storePtr);
    UnsignedSysInt currentOffset = 0;
    for(SysInt i = 0; i < (SysInt)stored_blocks.size(); ++i) {
      P((void*)(storePtr + currentOffset)
        << " " << (void*)stored_blocks[i].base << " " << stored_blocks[i].size);
      memcpy(storePtr + currentOffset, stored_blocks[i].base, stored_blocks[i].size);
      currentOffset += stored_blocks[i].size;
    }

    for(SysInt i = 0; i < (SysInt)extendable_blocks.size(); ++i) {
      memcpy(storePtr + currentOffset, extendable_blocks[i].base, extendable_blocks[i].size);
      currentOffset += extendable_blocks[i].size;
    }

    D_ASSERT(getDataSize() == currentOffset);
  }

private:
  void copyMemBlock(char* location, const BacktrackData& data, size_t copyStart,
                    size_t copy_length) {
    D_ASSERT(data.total_bytes() >= copyStart + copy_length);
    // memcpy(location, data.base + copyStart, copy_length);

    size_t dataCopy = 0;
    // If these is some data to copy, then we do so. We write the code this way
    // to avoid UnsignedSysInt underflow.
    if(copyStart <= data.total_bytes())
      dataCopy = std::min(data.total_bytes() - copyStart, copy_length);

    memcpy(location, data.data + copyStart, dataCopy);
    memset(location + dataCopy, 0, copy_length - dataCopy);
  }

public:
  void retrieveFromPtr(const BacktrackData& storePtr) {
    P("RetrieveMem: ");
    UnsignedSysInt currentOffset = 0;
    for(SysInt i = 0; i < (SysInt)stored_blocks.size(); ++i) {
      copyMemBlock(stored_blocks[i].base, storePtr, currentOffset, stored_blocks[i].size);
      currentOffset += stored_blocks[i].size;
    }

    for(SysInt i = 0; i < (SysInt)extendable_blocks.size(); ++i) {
      memcpy(extendable_blocks[i].base, storePtr.data + currentOffset, extendable_blocks[i].size);
      currentOffset += extendable_blocks[i].size;
    }

    D_ASSERT(getDataSize() == currentOffset);
  }

  /// Returns the size of the allocated memory in bytes.
  UnsignedSysInt getDataSize() {
    return total_stored_bytes + allocated_extendable_bytes;
  }

  BackTrackMemory() : total_stored_bytes(0), allocated_extendable_bytes(0), block_cache(100) {
    // Force at least one block to exist
    reallocate(0);
    backtrack_stack.push_back(BacktrackData{});
  }

  ~BackTrackMemory() {
    for(SysInt i = 0; i < (SysInt)backtrack_stack.size(); ++i)
      block_cache.do_free(backtrack_stack[i].data);
  }

  /// Copies the current state of backtrackable memory.
  void worldPush() {
    UnsignedSysInt dataSize = this->getDataSize();
    char* tmp = (char*)block_cache.do_malloc(dataSize); // calloc(dataSize, sizeof(char));
    this->copyIntoPtr(tmp);
    vector<size_t> extendable_blocksSize;
    for(int i = 0; i < extendable_blocks.size(); ++i) {
      extendable_blocksSize.push_back(extendable_blocks[i].size);
    }

    BacktrackData bd{total_stored_bytes, allocated_extendable_bytes, extendable_blocksSize, tmp};
    backtrack_stack.push_back(bd);
  }

  /// Restores the state of backtrackable memory to the last stored state.
  void worldPop() {
    D_ASSERT(backtrack_stack.size() > 0);
    BacktrackData bd = backtrack_stack.back();
    backtrack_stack.pop_back();
    D_ASSERT(total_stored_bytes >= bd.total_stored_bytes);

    // Clean up stored_blocks
    if(total_stored_bytes > bd.total_stored_bytes) {
      while(total_stored_bytes - stored_blocks.back().size > bd.total_stored_bytes) {
        BlockDef block = stored_blocks.back();
        stored_blocks.pop_back();
        free(block.base);
        total_stored_bytes -= block.size;
      }
      if(total_stored_bytes > bd.total_stored_bytes) {
        D_ASSERT(total_stored_bytes - stored_blocks.back().size <= bd.total_stored_bytes);
        size_t oldSize = stored_blocks.back().size;
        size_t diff = total_stored_bytes - bd.total_stored_bytes;
        size_t newSize = oldSize - diff;
        memset(stored_blocks.back().base + newSize, 0, diff);
        stored_blocks.back().size = newSize;
        total_stored_bytes -= diff;
      }
    }

    D_ASSERT(total_stored_bytes == bd.total_stored_bytes);

    // If you trigger this, ask Chris and we can generalise!
    D_ASSERT(extendable_blocks.size() == bd.extendable_blocksSize.size());
    for(int i = 0; i < extendable_blocks.size(); ++i) {
      if(bd.extendable_blocksSize[i] != extendable_blocks[i].size) {
        D_ASSERT(bd.extendable_blocksSize[i] < extendable_blocks[i].size);
        size_t diff = extendable_blocks[i].size - bd.extendable_blocksSize[i];
        extendable_blocks[i].size = bd.extendable_blocksSize[i];
        memset(extendable_blocks[i].base + extendable_blocks[i].size, 0, diff);
        allocated_extendable_bytes -= diff;
      }
    }

    D_ASSERT(allocated_extendable_bytes == bd.allocated_extendable_bytes);

    this->retrieveFromPtr(bd);
    block_cache.do_free(bd.data);
  }

  /// Returns the current number of stored copies of the state.
  SysInt current_depth() {
    return backtrack_stack.size();
  }

  void worldPopTo(SysInt depth) {
    D_ASSERT(current_depth() >= depth);
    while(current_depth() > depth)
      worldPop();
    D_ASSERT(current_depth() == depth);
  }

  /// Request a new block of memory and returns a \ref void* to it's start.
  void* request_bytes(DomainInt byteCount) {
    P("Request: " << (void*)this << " : " << byteCount);
    if(byteCount == 0)
      return NULL;

    // TODO: is the following line necessary?
    if(byteCount % sizeof(SysInt) != 0)
      byteCount += sizeof(SysInt) - (byteCount % sizeof(SysInt));

    total_stored_bytes += checked_cast<size_t>(byteCount);

    if(stored_blocks.back().remaining_capacity() < byteCount) {
      reallocate(byteCount);
    }

    D_ASSERT(stored_blocks.back().remaining_capacity() >= byteCount);
    void* returnVal = stored_blocks.back().base + stored_blocks.back().size;
    P("Return val:" << (void*)stored_blocks.back().base);
    stored_blocks.back().size += checked_cast<size_t>(byteCount);
    return returnVal;
  }

  ExtendableBlock requestBytesExtendable(UnsignedSysInt baseSize) {
    const SysInt maxSize = 512 * 1024 * 1024;
    char* block = (char*)calloc(maxSize, 1);
    extendable_blocks.push_back(BlockDef{block, baseSize, maxSize});
    allocated_extendable_bytes += baseSize;
    return ExtendableBlock{block, (SysInt)extendable_blocks.size() - 1};
  }

  void resizeExtendableBlock(ExtendableBlock block, UnsignedSysInt newSize) {
    UnsignedSysInt oldSize = extendable_blocks[block.getPos()].size;
    D_ASSERT(block() == extendable_blocks[block.getPos()].base);
    D_ASSERT(newSize >= oldSize);
    D_CHECK(newSize <= extendable_blocks[block.getPos()].capacity);
    D_ASSERT(checkAllZero(block() + oldSize, block() + newSize));

    allocated_extendable_bytes += (newSize - oldSize);

    extendable_blocks[block.getPos()].size = newSize;
    // block_resizes.back().push_back(make_tuple(block.getPos(), oldSize, newSize));
  }

  /// Request a \ref MoveableArray.
  template <typename T>
  T* requestArray(DomainInt size) {
    return (T*)request_bytes(size * sizeof(T));
  }

private:
  void reallocate(DomainInt byteCount_new_request) {
    P("Reallocate: " << (void*)this << " : " << byteCount_new_request);
    D_ASSERT(stored_blocks.size() == 0 ||
             (stored_blocks.back().remaining_capacity() < byteCount_new_request));

    size_t new_block_capacity = max(BLOCK_SIZE, checked_cast<size_t>(byteCount_new_request));
    char* base = (char*)calloc(new_block_capacity, sizeof(char));
    if(base == NULL) {
      D_FATAL_ERROR("calloc failed - Memory exhausted! Aborting.");
    }
    BlockDef bd{base, 0, new_block_capacity};
    stored_blocks.push_back(bd);
  }
};

// @}

#endif
