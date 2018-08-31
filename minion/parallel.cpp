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

struct ParallelData {
    sem_t processCount;
    sem_t outputLock;
};

void endParallelMinion() {
    sem_post(&(getParallelData().processCount));
    while (wait(0) > 0);
}

ParallelData* setupParallelData() {
    ParallelData* pd;
    pd = (ParallelData*)mmap(NULL, sizeof(ParallelData), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if(pd == MAP_FAILED) {
        D_FATAL_ERROR("Parallel data setup failed");
    }
    int cores = getOptions().parallelcores;
    if(cores < 1) {
        cores = sysconf( _SC_NPROCESSORS_ONLN );
    }
    if(sem_init(&(pd->processCount), 1, cores) != 0) {
        D_FATAL_ERROR("Setup semaphore fail");
    }

    if(sem_init(&(pd->outputLock), 1, 1) != 0) {
        D_FATAL_ERROR("Setup outputLock semaphore fail");
    }

    atexit(endParallelMinion);

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

bool shouldDoParallelFork() {
    if(!getOptions().parallel)
        return false;
    //int val = -2;
    //sem_getvalue(&(getParallelData().processCount), &val);
    //std::cout << "semaphore:" << val << "\n";
    bool ret = (sem_trywait(&(getParallelData().processCount)) == 0);
    //std::cout << "Checking semaphore:" << ret << strerror(errno) << "\n";
    return ret;
}


#else

struct ParallelData {

};

ParallelData* setupParallelData() {
    static ParallelData dummy;
    return &dummy;
}

#endif
