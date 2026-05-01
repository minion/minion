// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

// Per-worker shared-memory layout and partitioning helpers for parallel
// SAC / SACBounds preprocessing. The actual fork/merge driver
// (runParallelSACFixpoint) is templated and lives in preprocess.h, so this
// header only carries non-templated infrastructure.

#ifndef PREPROCESS_PARALLEL_H_QWERTYU
#define PREPROCESS_PARALLEL_H_QWERTYU

#include "../system/system.h"

#include <atomic>
#include <cstdint>
#include <vector>

namespace ParallelSAC {

// Status values written by a worker to its slot before _exit. The atomic is
// only used so the value is observable by the parent after pipe-EOF; there is
// no concurrent writer per slot.
enum WorkerStatus : uint32_t {
  STATUS_RUNNING = 0,
  STATUS_OK = 1,
  STATUS_EOS = 2,    // EndOfSearch (timeout / ctrl-C)
  STATUS_FATAL = 3,  // capacity overrun or unhandled exception
};

// Kind of an explicit pruning recorded by a worker.
enum PruneKind : int32_t {
  KIND_SET_MIN = 0,
  KIND_SET_MAX = 1,
  KIND_REMOVE_VAL = 2,
};

struct PruneEntry {
  uint32_t var_idx;
  int32_t kind;
  int64_t a;
};

struct WorkerSlotHeader {
  std::atomic<uint32_t> status;
  uint32_t entry_count;
  uint32_t capacity;
  uint32_t failed;  // 1 if worker observed getState().isFailed() before exit
};

inline PruneEntry* slotEntries(WorkerSlotHeader* h) {
  return reinterpret_cast<PruneEntry*>(h + 1);
}

// Append a prune entry. Returns false if capacity exceeded; in that case the
// status is bumped to STATUS_FATAL and subsequent appends are no-ops.
inline bool slotAppend(WorkerSlotHeader* slot, uint32_t var_idx, int32_t kind, int64_t a) {
  if(slot->entry_count >= slot->capacity) {
    slot->status.store(STATUS_FATAL);
    return false;
  }
  PruneEntry& e = slotEntries(slot)[slot->entry_count++];
  e.var_idx = var_idx;
  e.kind = kind;
  e.a = a;
  return true;
}

// Allocate one MAP_SHARED|MAP_ANON region holding a WorkerSlotHeader followed
// by `capacity` PruneEntries. The region is shared across fork; both parent
// and child see the same virtual address. Aborts on mmap failure.
WorkerSlotHeader* allocateSlot(uint32_t capacity);

// Release a slot's mmap region.
void releaseSlot(WorkerSlotHeader* slot);

// Partition variable indices [0..literalCount.size()) across `numWorkers`
// using a Largest Processing Time first heuristic: sort by literal count
// descending, then deal cyclically. Returns numWorkers vectors of indices.
// If numWorkers <= 0, behaves as 1.
std::vector<std::vector<SysInt>>
partitionByLiteralCount(const std::vector<long long>& literalCount, int numWorkers);

}  // namespace ParallelSAC

#endif
