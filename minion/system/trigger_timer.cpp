// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include <atomic>
#include <iostream>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

std::atomic<bool>* trig;
std::atomic<bool>* ctrlCPress;

bool checkDoubleCtrlc;

#include <stdio.h>

#ifdef _WIN32

#define _WIN32_WINNT 0x0500
#include <windows.h>

void CALLBACK TimerProc(void*, BOOLEAN) {
  *trig = true;
}

void CALLBACK ReallyStop(void*, BOOLEAN) {
  exit(1);
}

void activateTrigger(std::atomic<bool>* b, bool timeoutActive, int timeout, bool CPUTime) {
  if(CPUTime)
    cerr << "CPU-time timing not available on windows, falling back on clock" << endl;

  trig = b;
  *trig = false;

  HANDLE mTimerHandle;

  if(timeoutActive) {
    if(timeout <= 0)
      *trig = true;
    BOOL success = ::CreateTimerQueueTimer(&mTimerHandle, NULL, TimerProc, NULL, timeout * 1000, 0,
                                           WT_EXECUTEINTIMERTHREAD);
  }
}

void install_ctrlcTrigger(std::atomic<bool>*) { /* Not implemented on windows */
}

#else
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

void triggerFunction(int /* signum */) {
  *trig = true;
}

void activateTrigger(std::atomic<bool>* b, bool timeoutActive, int timeout,
                      bool CPUTime) // CPUTime = false -> real time
{
  // We still set these, as they are how 'ctrlc' checks if we have got started
  // properly or not.
  trig = b;
  *trig = false;

  signal(SIGXCPU, triggerFunction);
  signal(SIGALRM, triggerFunction);
  if(timeoutActive) {
    if(timeout <= 0)
      *trig = true;
    if(CPUTime) {
      rlimit lim;
      lim.rlim_cur = timeout;
      lim.rlim_max = timeout + 5;
      setrlimit(RLIMIT_CPU, &lim);
    } else
      alarm(timeout);
  }
}

void ctrlCFunction(int /* signum */) {
  if(checkDoubleCtrlc) {
    cerr << "Ctrl+C pressed twice. Exiting immediately." << endl;
    exit(1);
  }

  if(trig == NULL) {
    cerr << "Search has not started. Exiting immediately." << endl;
    exit(1);
  }

  checkDoubleCtrlc = true;

  cerr << getpid() << ": Ctrl+C pressed. Exiting." << std::endl;
  // This is the quickest way to get things to stop.
  *trig = true;
  *ctrlCPress = true;
}

void install_ctrlcTrigger(std::atomic<bool>* ctrlCPress_) {
  checkDoubleCtrlc = false;
  ctrlCPress = ctrlCPress_;
  signal(SIGINT, ctrlCFunction);
}
#endif
