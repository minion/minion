// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef _NONBACKTRACK_MEMORY_H
#define _NONBACKTRACK_MEMORY_H

#include "MemoryBlock.h"

// \addtogroup Memory
// @{

/// Encapsulates both the backtrackable and nonbacktrackable memory of a CSP
/// instance.
class Memory {
  BackTrackMemory backtrack_memory;
  MonotonicSet monotonic_set;

public:
  BackTrackMemory& backTrack() {
    return backtrack_memory;
  }
  MonotonicSet& monotonicSet() {
    return monotonic_set;
  }
};

// @}

#endif
