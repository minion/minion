// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include "minion.h"

#include "parallel/parallel.h"

// Disable on windows
#ifndef _WIN32
#define PARALLEL
#endif

#include <atomic>
#include <mutex>
#include <signal.h>
#include <errno.h>

namespace Parallel {

// Serialises concurrent setupThreadParallelData / releaseThreadParallelData
// calls. The signal-handler setters in system/trigger_timer.cpp store into
// process-static pointers (`trig`, `ctrlCPress`); racing on those from two
// concurrent threaded runs corrupts the dispatch state. The mutex covers the
// brief setup/teardown window only — the mutex is NOT held during search.
static std::mutex threadParallelDataMutex;

static bool checkIsAChildProcess;
static bool forkEverCalled;
static bool sigchldHandlerInstalled = false;

// This pipe is just to figure out when all children have exited, because
// when all children exit, the pipe will automatically close
static int childTrackingPipe[2];

bool isAChildProcess() {
  return checkIsAChildProcess;
}

bool isCtrlCPressed() {
  return getParallelData().ctrlCPressed;
}


bool shouldDoFork() {
  if(!getOptions().parallel)
    return false;
  if(getParallelData().processCount > 0) {
    int old = atomic_fetch_sub(&(getParallelData().processCount), 1);
    if(old < 0) {
      atomic_fetch_add(&(getParallelData().processCount), 1);
      return false;
    }
    return true;
  } else {
    return false;
  }
}

bool isAlarmActivated() {
  return getParallelData().alarmTrigger;
}

void setupAlarm(bool alarmActive, SysInt timeout, bool CPUTime) {
  activateTrigger(&(getParallelData().alarmTrigger), alarmActive, timeout, CPUTime);
}

} // namespace Parallel

#ifdef PARALLEL

#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace Parallel {

void lockSolsout() {
  if(getOptions().parallel || getOptions().numParallelThreads > 0 ||
     getOptions().workStealController != nullptr) {
    pthread_mutex_lock(&(getParallelData().outputLock));
  }
}

void unlockSolsout() {
  if(getOptions().parallel || getOptions().numParallelThreads > 0 ||
     getOptions().workStealController != nullptr) {
    pthread_mutex_unlock(&(getParallelData().outputLock));
  }
}

void setNumberCores(int cores) {
  if(cores > 0) {
    getParallelData().processCount = cores;
    getParallelData().initialProcessCount = cores;
  }
}

ParallelData* setupParallelData() {

  ParallelData* pd = 0;
  pd = (ParallelData*)mmap(NULL, sizeof(ParallelData), PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_ANON, -1, 0);
  if(pd == MAP_FAILED) {
    D_FATAL_ERROR("Parallel data setup failed");
  }
  int cores = sysconf(_SC_NPROCESSORS_ONLN);
  pd->processCount = cores;
  pd->initialProcessCount = cores;

  {
    pthread_mutexattr_t mutexAttr ;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    if(pthread_mutex_init(&(pd->outputLock), &mutexAttr) < 0) {
      D_FATAL_ERROR("Setup outputLock mutex fail");
    }
  }

  install_ctrlcTrigger(&(pd->ctrlCPressed));

  pd->parentProcessID = getpid();

  return pd;
}

ParallelData* setupThreadParallelData() {
  std::lock_guard<std::mutex> lock(threadParallelDataMutex);

  ParallelData* pd = new ParallelData();
  pd->processCount = 0;
  pd->initialProcessCount = 0;
  pd->fatalErrorOccurred = false;
  pd->process_should_exit = false;
  pd->solutions = 0;
  pd->nodes = 0;
  pd->children = 0;
  pd->ctrlCPressed = false;
  pd->alarmTrigger = false;

  {
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    if(pthread_mutex_init(&(pd->outputLock), &mutexAttr) != 0) {
      D_FATAL_ERROR("Setup thread-mode outputLock mutex fail");
    }
  }

  install_ctrlcTrigger(&(pd->ctrlCPressed));
  pd->parentProcessID = getpid();
  return pd;
}

void releaseThreadParallelData(ParallelData* pd) {
  if(pd == nullptr)
    return;
  std::lock_guard<std::mutex> lock(threadParallelDataMutex);
  pthread_mutex_destroy(&(pd->outputLock));
  delete pd;
}

void setupParallelSupport() {
  // make sure we don't end up with too many children
  signal(SIGCHLD, SIG_IGN);

  // Setup a pipe so parent can track if children are alive
  int ret = pipe(childTrackingPipe);

  if(ret < 0) {
    D_FATAL_ERROR("Parallel pipe construction failed");
  }

}


int doFork() {
  if(!forkEverCalled) {
    setupParallelSupport();
  }
  forkEverCalled = true;
  int f = fork();
  if(f < 0) {
    D_FATAL_ERROR("Fork fail!\n");
    getParallelData().fatalErrorOccurred = true;
  } else if(f == 0) {
    getState().resetSearchCounters();
    if(!checkIsAChildProcess) {
      checkIsAChildProcess = true;
      getOptions().silent = true;
      close(childTrackingPipe[0]);
      // std::cout << getpid() << " closing 0" << std::endl;
      // int devNull = open("/dev/null", O_WRONLY);
      // dup2(devNull, 1);
      // devNull = open("/dev/null", O_RDONLY);
      // dup2(devNull, 0);
    }
  }
  return f;
}

void ensureSignalHandlersInstalled() {
  if(!sigchldHandlerInstalled) {
    signal(SIGCHLD, SIG_IGN);
    sigchldHandlerInstalled = true;
  }
}

RoundHandle beginRound() {
  ensureSignalHandlersInstalled();
  RoundHandle h;
  if(pipe(h.pipeFds) < 0) {
    D_FATAL_ERROR("Round pipe construction failed");
  }
  return h;
}

int forkForRound(RoundHandle& handle) {
  int f = fork();
  if(f < 0) {
    D_FATAL_ERROR("Fork failed for parallel preprocess round");
  } else if(f == 0) {
    // Child: close the read end; we never read. Suppress per-worker chatter so
    // each child doesn't independently print things like the SAC timeout line.
    close(handle.pipeFds[0]);
    getOptions().silent = true;
  }
  return f;
}

void endRound(RoundHandle& handle) {
  // Close the parent's copy of the write end so EOF can occur after all
  // children also close theirs (which they do automatically on _exit).
  close(handle.pipeFds[1]);
  char buf[1024];
  while(true) {
    ssize_t r = read(handle.pipeFds[0], buf, sizeof(buf));
    if(r == 0)
      break;
    if(r < 0 && errno == EINTR)
      continue;
    if(r < 0)
      D_FATAL_ERROR("Round pipe read failed");
  }
  close(handle.pipeFds[0]);
}

void endParallelMinion() {
  if(!forkEverCalled)
    return;
  atomic_fetch_add(&(getParallelData().processCount), 1);

  if(checkIsAChildProcess) {
    atomic_fetch_add(&(getParallelData().solutions), getState().getSolutionCount());
    atomic_fetch_add(&(getParallelData().nodes), getState().getNodeCount());
    atomic_fetch_add(&(getParallelData().children), (long long)1);
  }

  if(!checkIsAChildProcess) {
    std::cout << "Waiting for all child processes to exit.." << std::endl;
    // Don't close until now, so all children have this pipe
    close(childTrackingPipe[1]);

    signal(SIGPIPE, SIG_IGN);
    int ret = 1;
    while(ret != 0) {
      char buf[1024];
      // std::cout << getpid() << " reading 0" << std::endl;
      ret = read(childTrackingPipe[0], buf, 1024);
      // std::cout << ret << std::endl;
      // perror("Error:");
      // std::cerr << "Ready loop" << std::endl;
    }
    if(getParallelData().fatalErrorOccurred) {
      std::cerr << "**ERROR ERROR ERROR ERROR***\n";
      std::cerr << "** A Fatal error occurred during parallelisation **\n";
      exit(1);
    }

    if(getParallelData().processCount != getParallelData().initialProcessCount + 1) {
      std::cerr << "**ERROR ERROR ERROR ERROR***\n";
      std::cerr << "**AT LEAST ONE CHILD PROCESS WAS LOST**\n";
      std::cerr << "**THE SEARCH IS INCOMPLETE**\n";
      exit(1);
    }

    std::cout << "A total of " << getParallelData().children << " children were used" << std::endl;

    getState().incrementSolutionCount(getParallelData().solutions);
    getState().incrementNodeCount(getParallelData().nodes);
  }
}

} // namespace Parallel

#else

namespace Parallel {

void lockSolsout() {
}

void unlockSolsout() {
}

void setNumberCores(int cores) {
}


void endParallelMinion() {}

int doFork() {
  D_FATAL_ERROR("This Minion was built without parallelisation");
}

void ensureSignalHandlersInstalled() {}

RoundHandle beginRound() {
  D_FATAL_ERROR("Parallel preprocess not supported on this platform");
}

int forkForRound(RoundHandle&) {
  D_FATAL_ERROR("Parallel preprocess not supported on this platform");
}

void endRound(RoundHandle&) {}

ParallelData* setupParallelData() {
#ifdef LIBMINION
  static thread_local ParallelData dummy;
#else
  static ParallelData dummy;
#endif
  return &dummy;
}

ParallelData* setupThreadParallelData() {
  ParallelData* pd = new ParallelData();
  pd->processCount = 0;
  pd->initialProcessCount = 0;
  pd->fatalErrorOccurred = false;
  pd->process_should_exit = false;
  pd->solutions = 0;
  pd->nodes = 0;
  pd->children = 0;
  pd->ctrlCPressed = false;
  pd->alarmTrigger = false;
  return pd;
}

void releaseThreadParallelData(ParallelData* pd) {
  delete pd;
}
} // namespace Parallel
#endif
