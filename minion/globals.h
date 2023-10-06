#ifndef _GLOBALS_H
#define _GLOBALS_H

#include "StateObj_forward.h"
#include <fstream>
#include <random>

struct Globals {
  Memory* searchMem_m;
  SearchOptions* options_m;
  SearchState* state_m;
  Queues* queues_m;
  VariableContainer* varContainer_m;
  BoolContainer* bools_m;
  Parallel::ParallelData* parData_m;
  std::mt19937 global_random_gen;
  std::ofstream solsoutfile;
  /*
   * Pointer trickery as compiler doesnt like globals.x when there are still
   * incomplete types (such as SearchOptions, ...).
   * Tried rearranging headerfiles, didn't work, so am lazily creating them when referenced.
   * instead in StateObj.hpp.
   */

public:
  Globals() {
    std::mt19937 global_random_gen;
    std::ofstream solsoutfile;
    Memory* searchMem_m = NULL;
    SearchOptions* options_m = NULL;
    SearchState* state_m = NULL;
    Queues* queues_m = NULL;
    VariableContainer* varContainer_m = NULL;
    BoolContainer* bools_m = NULL;
    Parallel::ParallelData* parData_m = NULL;
  }

  ~Globals() {
    /*
     * FIXME: 
     * Unfortunately can't be deconstructed properly - 
     * see libwrapper.cpp:resetMinion
     */
  }
};

#ifdef LIBMINION
extern Globals* globals;
#endif

#endif
