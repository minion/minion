// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef _PARALLEL_H_RQEUOERFOUJFDNJLFD
#define _PARALLEL_H_RQEUOERFOUJFDNJLFD

#include <atomic>

namespace Parallel {
struct ParallelData {

  std::atomic<int> processCount;
  int initialProcessCount;
  pthread_mutex_t outputLock;

  std::atomic<bool> fatalErrorOccurred;
  std::atomic<bool> process_should_exit;
  std::atomic<long long> solutions;
  std::atomic<long long> nodes;
  std::atomic<long long> children;
  pid_t parentProcessID;
  std::atomic<bool> ctrlCPressed;
  std::atomic<bool> alarmTrigger;
};

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
} // namespace Parallel

#endif