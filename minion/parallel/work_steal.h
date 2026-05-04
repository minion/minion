// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0
//
// Thread-based work-stealing search infrastructure.
//
// Each worker has its own MinionContext + SearchManager and runs an
// independent DFS. When some worker is idle, busy workers periodically
// donate one stealable left-branch to the shared queue: they encode a
// path-from-root and mark the branch as stolen so they themselves stop
// visiting it on backtrack. Idle workers pop a path, fast-forward via
// branch_left + worldPush replay, and search the resulting sub-tree.
//
// All cross-thread state lives in WorkStealController (heap-allocated,
// std::mutex / std::condition_variable, no fork). Soundness relies on
// the fact that propagation is deterministic given the variable domains
// at a node, so a replayed prefix produces the same node state the
// donor had at the steal point. Wdeg counters DO drift between donor
// and stealer; this is accepted (the user prefers it as a diversity
// source).

#ifndef WORK_STEAL_H_QWERTYU
#define WORK_STEAL_H_QWERTYU

#include "../system/system.h"
#include "../inputfile_parse/InputVariableDef.h"

#include <atomic>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <queue>
#include <vector>

namespace WorkSteal {

// One step in a path from the root to a stolen sub-tree. `var` identifies
// a variable by (type, pos) — stable across worker contexts because each
// worker built its variables in identical order from the same CSPInstance.
// `effectiveLeft = true` means "var = value" was applied; `false` means
// "var != value" (right branch). `value` is stored as int64 for portability
// under -domains64.
struct BranchStep {
  uint8_t varType;  // VariableType cast to byte
  uint32_t varPos;
  int64_t value;
  bool effectiveLeft;
};

struct WorkItem {
  std::vector<BranchStep> path;
};

// Shared coordinator state for one parallel work-stealing run. One
// instance lives on the parent thread's stack; pointer captured by each
// worker. All atomics/mutexes are heap-allocated (not mmap), good for
// cross-thread use.
struct WorkStealController {
  // Work distribution.
  std::mutex queue_mutex;
  std::condition_variable queue_cv;
  std::queue<WorkItem> queue;

  // Coordination atomics. busyWorkers + idleWorkers + (queue empty/full)
  // determine when the search is complete: all workers idle AND queue
  // empty AND no donation in flight.
  std::atomic<int> busyWorkers;   // workers currently searching
  std::atomic<int> idleWorkers;   // workers blocked on queue_cv
  std::atomic<int> donationInFlight;  // donations between dequeue and busy++

  // Total worker count (set once, read-only after spawn).
  int numWorkers;

  // Termination flag. Set by a worker that found enough solutions, by
  // the caller via signal handler, etc.
  std::atomic<bool> stopRequested;

  // Diagnostics counters (for debugging/observability; cheap to read).
  std::atomic<long long> donationsMade;
  std::atomic<long long> workItemsTaken;
  std::atomic<long long> replayFailures;  // path was infeasible

  // Contention diagnostics. All in nanoseconds, summed across all
  // workers for the lifetime of the run. Each is the cumulative wall
  // time spent in a specific blocking primitive — e.g. if eight
  // workers each waited 100 ms on the queue mutex, queueLockWaitNanos
  // is 800 ms. Divide by (numWorkers * wallTimeNanos) for "fraction
  // of available CPU spent here". Captured via steady_clock probes
  // around lock/wait acquisitions; see work_steal.cpp.
  //
  //   queueLockWaitNanos:    blocking on queue_mutex (donate +
  //                          popOrFinish acquire). High value means
  //                          workers are contending for the donation
  //                          queue itself.
  //   idleWaitNanos:         time blocked in queue_cv.wait inside
  //                          popOrFinish — i.e. worker had no work
  //                          and was waiting for a donation. High
  //                          value means donation supply isn't
  //                          keeping up with worker demand.
  //   callbackLockWaitNanos: blocking on callbackMutex (per-solution
  //                          serialisation in the wrapper callback,
  //                          libwrapper.cpp). High value means
  //                          solution-output is the bottleneck —
  //                          common on enumeration-heavy SAT runs.
  std::atomic<long long> queueLockWaitNanos;
  std::atomic<long long> idleWaitNanos;
  std::atomic<long long> callbackLockWaitNanos;

  // Aggregated across all workers, for the parent's TableOut /
  // CLI summary. Workers add their per-context counts on exit.
  std::atomic<long long> totalNodesExplored;

  // Cross-worker optimisation bound channel. Workers push their
  // post-bump optVals[0] here on each solution (CAS-max) and read
  // it back during opt_handler to tighten further than their own
  // local bound. Direction: minion internally maximises
  // OptimiseVars (negating user-side minimisation), so "tighter" =
  // LARGER. Single-objective only — multi-objective lex
  // optimisation needs a vector and is skipped silently. The
  // sentinel "no bound seen yet" is LLONG_MIN.
  std::atomic<long long> sharedOptBound;

  // Solution accounting (mirrors ParallelController's contract).
  std::atomic<long long> totalSolutionsFound;
  std::mutex callbackMutex;
  long long sollimit;  // -1 = find all
  bool (*userCallback)(void* /* MinionContext */, void*);
  void* userUserdata;

  WorkStealController()
      : busyWorkers(0),
        idleWorkers(0),
        donationInFlight(0),
        numWorkers(0),
        stopRequested(false),
        donationsMade(0),
        workItemsTaken(0),
        replayFailures(0),
        queueLockWaitNanos(0),
        idleWaitNanos(0),
        callbackLockWaitNanos(0),
        totalNodesExplored(0),
        sharedOptBound(std::numeric_limits<long long>::min()),
        totalSolutionsFound(0),
        sollimit(-1),
        userCallback(nullptr),
        userUserdata(nullptr) {}
};

// Pop work from the queue, blocking until either a WorkItem arrives or
// the controller decides we're done. Returns true on dequeue (out is
// filled), false when the worker should exit.
//
// Must be called with the worker NOT counted in busyWorkers (i.e., after
// the worker has finished its previous sub-tree and decremented busy).
// On true return, the worker has incremented busy and decremented the
// donationInFlight counter; the caller proceeds to replay+search.
bool popOrFinish(WorkStealController& c, WorkItem& out);

// Called by the very first worker (worker 0) to bootstrap: it counts
// itself busy and starts at the root, no replay.
void bootstrapFirstWorker(WorkStealController& c);

// Called by a worker that just finished its current sub-tree (search loop
// returned). Decrements busy, increments idle, and signals other workers
// in case they're awaiting the all-done condition.
void markIdle(WorkStealController& c);

// Donate one stealable branch to the queue. Returns true if a branch was
// donated (caller should also have set branches[steal].stolen = true on
// its own SearchManager). `path` is the encoded path from root to the
// right side of branches[steal].
//
// donationInFlight is incremented by this function and decremented by
// the matching popOrFinish that picks the item up — this closes the
// "all idle, queue empty, but a donation is mid-push" race.
void donate(WorkStealController& c, WorkItem item);

// Convert an AnyVarRef → (varType, pos) for path encoding. Calls
// AnyVarRef::getBaseVar() under the hood.
//
// Defined inline in work_steal_impl.h (after AnyVarRef.h is available).

}  // namespace WorkSteal

#endif
