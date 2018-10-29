/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
* USA.
*/

#include "minion.h"
#include "parallel/parallel.h"

#define PARALLEL

#ifdef PARALLEL

#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <atomic>

namespace Parallel {

struct ParallelData {

    std::atomic<int> processCount;
    
    sem_t outputLock;
    
    std::atomic<bool> fatal_error_occurred;
    std::atomic<bool> process_should_exit;
    std::atomic<long long> solutions;
    std::atomic<long long> nodes;
    std::atomic<long long> children;
    pid_t parent_process_id;
    std::atomic<bool> ctrl_c_pressed;
    std::atomic<bool> alarm_trigger;
};

static bool is_a_child_process;
static bool fork_ever_called;

// This pipe is just to figure out when all children have exited, because
// when all children exit, the pipe will automatically close
static int child_tracking_pipe[2];

bool isAChildProcess() {
    return is_a_child_process;
}

bool isCtrlCPressed() {
    std::cerr << getpid() << "checking ctrl+c" << getParallelData().ctrl_c_pressed << "\n";
    return getParallelData().ctrl_c_pressed;
}

void endParallelMinion() {
    if(!fork_ever_called) return;
    atomic_fetch_add(&(getParallelData().processCount), 1);

    if(is_a_child_process) {
        atomic_fetch_add(&(getParallelData().solutions), getState().getSolutionCount());
        atomic_fetch_add(&(getParallelData().nodes), getState().getNodeCount());
        atomic_fetch_add(&(getParallelData().children), (long long)1);
    }

    if(!is_a_child_process) {
        std::cout << "Waiting for all child processes to exit.." << std::endl;
        // Don't close until now, so all children have this pipe
        close(child_tracking_pipe[1]);

        signal(SIGPIPE, SIG_IGN);
        int ret = 1;
        while(ret != 0) {
            char buf[1024];
            //std::cout << getpid() << " reading 0" << std::endl;
            ret = read(child_tracking_pipe[0], buf, 1024);
            //std::cout << ret << std::endl;
            //perror("Error:");
            //std::cerr << "Ready loop" << std::endl;
        }
        if(getParallelData().fatal_error_occurred) {
            std::cerr << "ERROR: A Fatal error occurred during parallelisation\n";
            exit(1);
        }
        std::cout << "A total of " << getParallelData().children << " children were used" << std::endl;

        getState().incrementSolutionCount(getParallelData().solutions);
        getState().incrementNodeCount(getParallelData().nodes);
    }
}


ParallelData* setupParallelData() {
    // Setup a pipe so parent can track if children are alive
    pipe(child_tracking_pipe);

    ParallelData* pd;
    pd = (ParallelData*)mmap(NULL, sizeof(ParallelData), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if(pd == MAP_FAILED) {
        D_FATAL_ERROR("Parallel data setup failed");
    }
    int cores = getOptions().parallelcores;
    if(cores < 1) {
        cores = sysconf( _SC_NPROCESSORS_ONLN );
    }
    pd->processCount = cores;

    if(sem_init(&(pd->outputLock), 1, 1) != 0) {
        D_FATAL_ERROR("Setup outputLock semaphore fail");
    }

    install_ctrlc_trigger(&(pd->ctrl_c_pressed));

    pd->parent_process_id = getpid();

    return pd;
}

void lockSolsout() {
    if(getOptions().parallel) {
        sem_wait(&(getParallelData().outputLock));
    }
}

void unlockSolsout() {
    if(getOptions().parallel) {
        sem_post(&(getParallelData().outputLock));
    }
}

bool shouldDoFork() {
    if(!getOptions().parallel)
        return false;
    if(getParallelData().processCount > 0)
    {
        int old = atomic_fetch_sub(&(getParallelData().processCount), 1);
        if(old < 0) {
            atomic_fetch_add(&(getParallelData().processCount), 1);
            return false;
        } 
        return true;
    }
    else {
        return false;
    }
}

int doFork() {
    fork_ever_called = true;
    int f = fork();
    if(f < 0) {
        D_FATAL_ERROR("Fork fail!\n");
        getParallelData().fatal_error_occurred = true;
    }
    else if(f == 0) {
        getState().resetSearchCounters();
        if(!is_a_child_process) {
            is_a_child_process = true;
            close(child_tracking_pipe[0]);
            //std::cout << getpid() << " closing 0" << std::endl;
            //int devNull = open("/dev/null", O_WRONLY);
            //dup2(devNull, 1);
            //devNull = open("/dev/null", O_RDONLY);
            //dup2(devNull, 0);
        }
    }
    return f;
}

bool isAlarmActivated() {
    return getParallelData().alarm_trigger;
}

  void setupAlarm(bool alarm_active, SysInt timeout, bool CPU_time) {
    activate_trigger(&(getParallelData().alarm_trigger), alarm_active, timeout, CPU_time);
  }


}
#else

namespace Parallel {
struct ParallelData {

};

ParallelData* setupParallelData() {
    static ParallelData dummy;
    return &dummy;
}
}
#endif