// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include <vector>

struct BlockCache {
  std::vector<char*> blocks;

  BlockCache(SysInt size) {
    blocks.resize(size);
  }

  char* do_malloc(size_t size) {
    // This is because realloc/malloc will sometimes return 0 with size=0
    if(size == 0)
      return (char*)(0);

    if(blocks.empty()) {
      char* ptr = static_cast<char*>(checked_malloc(size));
      if(ptr == NULL) {
        D_FATAL_ERROR("Malloc failed - Memory exausted! Aborting.");
      }
      return ptr;
    } else {
      char* ret = blocks.back();
      blocks.pop_back();
      char* ptr = static_cast<char*>(realloc(ret, size));
      if(ptr == NULL) {
        D_FATAL_ERROR("Realloc failed - Memory exausted! Aborting.");
      }
      return ptr;
    }
  }

  void do_free(char* ptr) {
    if(blocks.size() == blocks.capacity())
      free(ptr);
    else
      blocks.push_back(ptr);
  }

  ~BlockCache() {
    for(SysInt i = 0; i < (SysInt)blocks.size(); ++i)
      free(blocks[i]);
  }
};
