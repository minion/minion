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

#ifndef _BACKTRACK_MEMORY_H
#define _BACKTRACK_MEMORY_H

#include "MemoryBlock.h"
#include "../system/block_cache.h"
#include <utility>
#ifdef P
#undef P
#endif

#define P(x)
//#define P(x) std::cout << x << std::endl

// \addtogroup Memory
// @{

/// Provides a wrapper around \ref NewMemoryBlock for backtrackable memory.
/* This class acts like a stack, allowing the backtrackable state to be poped
 * and pushed as required during search.
 */
class BackTrackMemory {
  NewMemoryBlock new_memory_block;
  BlockCache block_cache;

  vector<pair<char*, UnsignedSysInt>> backtrack_data;

public:
  /// Wraps request_bytes of the internal \ref NewMemoryBlock.
  void* request_bytes(UnsignedSysInt byte_count) {
    return new_memory_block.request_bytes(byte_count);
  }

  ExtendableBlock requestBytesExtendable(UnsignedSysInt base_size) {
    return new_memory_block.requestBytesExtendable(base_size);
  }

  void resizeExtendableBlock(ExtendableBlock block, UnsignedSysInt new_size)
  { new_memory_block.resizeExtendableBlock(block, new_size); }
  
  /// Wraps requestArray of the internal \ref NewMemoryBlock.
  template <typename T>
  T* requestArray(UnsignedSysInt size) {
    return new_memory_block.requestArray<T>(size);
  }

  BackTrackMemory() : block_cache(100), backtrack_data() {}

  /// Copies the current state of backtrackable memory.
  void world_push() {
    UnsignedSysInt data_size = new_memory_block.getDataSize();
    char* tmp = (char*)block_cache.do_malloc(data_size); // calloc(data_size, sizeof(char));

    new_memory_block.storeMem(tmp);
    backtrack_data.push_back(std::make_pair(tmp, data_size));
  }

  /// Restores the state of backtrackable memory to the last stored state.
  void world_pop() {
    D_ASSERT(backtrack_data.size() > 0);
    pair<char*, size_t> tmp = backtrack_data.back();
    new_memory_block.retrieveMem(tmp);
    backtrack_data.pop_back();
    block_cache.do_free(tmp.first);
  }

  /// Returns the current number of stored copies of the state.
  SysInt current_depth() {
    return backtrack_data.size();
  }

  ~BackTrackMemory() {
    for(SysInt i = 0; i < (SysInt)backtrack_data.size(); ++i)
      block_cache.do_free(backtrack_data[i].first);
  }
};

#endif
