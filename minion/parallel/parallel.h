// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef _PARALLEL_H_RQEUOERFOUJFDNJLFD
#define _PARALLEL_H_RQEUOERFOUJFDNJLFD

#include <atomic>

namespace Parallel {
struct ParallelData {

  std::atomic<int> processCount;
  int initialProcessCount;
#ifndef WIN32
  pthread_mutex_t outputLock;
  pid_t parentProcessID;
#endif
  std::atomic<bool> fatalErrorOccurred;
  std::atomic<bool> process_should_exit;
  std::atomic<long long> solutions;
  std::atomic<long long> nodes;
  std::atomic<long long> children;
  std::atomic<bool> ctrlCPressed;
  std::atomic<bool> alarmTrigger;
};

void setNumberCores(int cores);
ParallelData* setupParallelData();
void lockSolsout();
void unlockSolsout();
bool shouldDoFork();
int doFork();
bool isAChildProcess();
bool isCtrlCPressed();
bool isAlarmActivated();
void setupAlarm(bool alarmActive, SysInt timeout, bool CPUTime);
void endParallelMinion();

// Heap-allocated (non-mmap) ParallelData used for thread-mode portfolio
// search. The mutex is initialised without PTHREAD_PROCESS_SHARED and the
// ctrl-C trigger is wired to point at this struct's atomic so a single
// signal-handler installation reaches all worker threads when several
// MinionContexts alias their parData_m to it.
ParallelData* setupThreadParallelData();
void releaseThreadParallelData(ParallelData* pd);

// Per-round fork machinery used by parallel SAC preprocessing. Independent of
// the search-time parallel pool: round forks do not consume the -cores budget
// and do not get tagged as search children. Unix-only; functions assert on
// platforms where fork is unavailable.
struct RoundHandle {
  int pipeFds[2];  // [0]=read (parent), [1]=write (kept open by children until _exit)
};

// Idempotent: install SIGCHLD ignore so children auto-reap without waitpid.
void ensureSignalHandlersInstalled();

// Allocate a fresh tracking pipe for the round. Call once per round in parent.
RoundHandle beginRound();

// fork(). Returns 0 in the child (which closes its copy of the read end) and
// the child's PID in the parent. Children should _exit when done so the
// parent's read() on pipeFds[0] returns EOF after the last child exits.
int forkForRound(RoundHandle& handle);

// Block in the parent until all children have closed pipeFds[1] (i.e. exited).
// Closes both fds before returning. Safe to call once per round.
void endRound(RoundHandle& handle);

} // namespace Parallel

#endif