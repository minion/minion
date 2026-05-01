// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "../minion.h"
#include "preprocess_parallel.h"

#include <algorithm>

#if !defined(_WIN32)
#include <sys/mman.h>
#endif

namespace ParallelSAC {

#if !defined(_WIN32)

WorkerSlotHeader* allocateSlot(uint32_t capacity) {
  size_t sz = sizeof(WorkerSlotHeader) + (size_t)capacity * sizeof(PruneEntry);
  void* p = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
  if(p == MAP_FAILED) {
    D_FATAL_ERROR("mmap failed allocating parallel-preprocess worker slot");
  }
  WorkerSlotHeader* h = static_cast<WorkerSlotHeader*>(p);
  // Placement-new the atomic to guarantee well-defined initialisation in the
  // shared region (raw mmap returns zeroed pages but std::atomic init is
  // technically required).
  new(&h->status) std::atomic<uint32_t>(STATUS_RUNNING);
  h->entry_count = 0;
  h->capacity = capacity;
  h->failed = 0;
  return h;
}

void releaseSlot(WorkerSlotHeader* slot) {
  if(slot == nullptr)
    return;
  size_t sz = sizeof(WorkerSlotHeader) + (size_t)slot->capacity * sizeof(PruneEntry);
  munmap(slot, sz);
}

#else

WorkerSlotHeader* allocateSlot(uint32_t) {
  D_FATAL_ERROR("Parallel preprocess not supported on this platform");
}

void releaseSlot(WorkerSlotHeader*) {}

#endif

std::vector<std::vector<SysInt>>
partitionByLiteralCount(const std::vector<long long>& literalCount, int numWorkers) {
  if(numWorkers <= 0)
    numWorkers = 1;

  std::vector<SysInt> sortedIdx;
  sortedIdx.reserve(literalCount.size());
  for(SysInt i = 0; i < (SysInt)literalCount.size(); ++i)
    sortedIdx.push_back(i);

  std::sort(sortedIdx.begin(), sortedIdx.end(), [&](SysInt a, SysInt b) {
    return literalCount[a] > literalCount[b];
  });

  std::vector<std::vector<SysInt>> partition(numWorkers);
  for(size_t k = 0; k < sortedIdx.size(); ++k) {
    partition[k % (size_t)numWorkers].push_back(sortedIdx[k]);
  }
  return partition;
}

}  // namespace ParallelSAC
