// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

namespace Parallel {
struct ParallelData;
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