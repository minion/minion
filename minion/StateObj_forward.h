// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef STATEOBJ_FORWARD
#define STATEOBJ_FORWARD

class BoolContainer;
class SearchOptions;
class SearchState;
class Queues;
class Memory;
class VariableContainer;

inline BoolContainer& getBools();
inline SearchOptions& getOptions();
inline SearchState& getState();
inline Queues& getQueue();
inline Memory& getMemory();
inline VariableContainer& getVars();

namespace Parallel {
struct ParallelData;
inline ParallelData& getParallelData();
} // namespace Parallel
#endif
