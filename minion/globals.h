#ifndef _GLOBALS_H
#define _GLOBALS_H

#include "StateObj_forward.h"
#include "variables/AnyVarRef.h"
#include <fstream>
#include <random>

class TableOut;

struct Globals {
  Memory* searchMem_m;
  SearchOptions* options_m;
  SearchState* state_m;
  Queues* queues_m;
  VariableContainer* varContainer_m;
  BoolContainer* bools_m;
  Parallel::ParallelData* parData_m;
  TableOut* tableOut_m;
  bool(*callback)(Globals*, void*);
  void* callbackUserdata;
  std::mt19937 global_random_gen;
  std::ofstream solsoutfile;
  /// Where this context's output goes. Defaults to std::cout, which is
  /// what the standalone binary wants (main.cpp builds a Globals
  /// directly). Library contexts are pointed at out_sink_m by
  /// minion_newContext instead, so nothing is written to the terminal
  /// and, crucially, the process-global std::cout is never touched --
  /// swapping cout's rdbuf races with every other thread that is
  /// printing, which garbles output and can fault outright when one
  /// thread writes while another has it null.
  std::ostream* out_m;

  /// Backing sink for a context that must not write to the terminal.
  /// Left unopened, so writes to it just set failbit and are discarded:
  /// nothing is buffered and nothing grows, unlike an ostringstream.
  /// Opened as a real file when LIBMINION_LOG is set.
  std::ofstream out_sink_m;
  /*
   * Pointer trickery as compiler doesnt like globals.x when there are still
   * incomplete types (such as SearchOptions, ...).
   * Tried rearranging headerfiles, didn't work, so am lazily creating them when referenced.
   * instead in StateObj.hpp.
   */

public:
  Globals();
  ~Globals();
};

#ifdef LIBMINION
// Thread-local to allow multiple concurrent Minion instances on different threads.
// Each thread sets this pointer to its own Globals before calling internal functions.
// MinionContext is an opaque handle exposed in the public API.
typedef Globals MinionContext;
extern thread_local Globals* globals;
#endif

#endif
