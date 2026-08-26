// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef _SOLVER_H
#define _SOLVER_H

#include "system/system.h"

#include "tuple_container.h"

#include "StateObj_forward.h"

#include "memory_management/GenericBacktracker.h"

#include "search_dump.hpp"

// Some advanced definitions, we don't actually need to know anything about
// these
// types for SearchState, simply that they exist.
class AbstractConstraint;
class AnyVarRef;

namespace Controller {
struct SearchManager;
} // namespace Controller

namespace ProbSpec {
struct CSPInstance;
}

class SearchState {

  long long nodes;
  long long backtracks;
  // Cumulative parallel-SAC fixpoint rounds executed across all
  // -X-parallelPreprocess invocations in this run. A "round" is one
  // fork-join wave where every worker independently re-derives SAC on
  // its slice, then prunings are merged into the parent. Rounds == 0
  // means parallel preprocess never fired (e.g. flag wasn't set, or
  // only ran sequential SAC). Rounds >= 2 means the first round found
  // prunings AND at least one further round was needed to prove
  // convergence. Surfaced via TableOut so the random tester can grow
  // instance size adaptively until the parallel path is actually
  // exercised, mirroring the work-steal sweep's adaptive-sizing
  // strategy.
  long long parallelPreprocessRounds = 0;
  // Total prunings (setMin + setMax + removeVal entries) emitted by
  // worker slots and applied to the parent across all rounds. A
  // direct "did parallel preprocess do useful work" signal — rounds
  // can be 1 with prunings == 0 if the instance was already SAC at
  // entry, in which case the parallel path ran but discovered
  // nothing.
  long long parallelPreprocessPrunings = 0;
  vector<AnyVarRef> optimiseVars;
  vector<AnyVarRef> raw_optimiseVars;
  vector<DomainInt> current_optimise_positions;
  // Raw values of the optimisation variables at the last non-dominated
  // solution. Set under the same lock as the "Solution found with Value:"
  // print in check_sol_is_correct, so it always reflects the
  // best-so-far solution that minion has actually committed to. Empty
  // until the first non-dominated solution is found. Read at end of
  // search to surface OptimumValue in TableOut.
  vector<DomainInt> last_optimum_values;
  bool optimise;
  bool maximise;

  // The variables to print when a solution is found.
  vector<vector<AnyVarRef>> print_matrix;

  vector<AbstractConstraint*> constraints;

  vector<set<AbstractConstraint*>> constraintsToPropagate;

  long long int solutions;

  bool finished;
  bool failed;

  ProbSpec::CSPInstance* csp_instance;

  TimerClass oldtimer;

  shared_ptr<TupleListContainer> tupleListContainer;
  shared_ptr<ShortTupleListContainer> shortTupleListContainer;


  GenericBacktracker generic_backtracker;

  // Raw pointer into the live SearchManager owned by the top-level driver
  // (BuildCSP.cpp). Used by minion_newVarMidsearch to route new vars into
  // the live search's aux block. Valid only between setSearchManager and
  // the end of sm->search(); nullptr otherwise.
  Controller::SearchManager* currentSearchManager = nullptr;

public:
  std::string storedSolution;

  // Set by check_sol_is_correct when running under
  // SearchOptions::parallelBoundChannel and the just-found solution's
  // post-bump optimisation value does NOT strictly improve on the
  // shared bound (i.e. another worker has already found at least as
  // good a solution). Read by standard_dealWith_solution to suppress
  // its "Solution found with Value: X" print, keeping the printed
  // sequence monotonically improving across workers. Reset on every
  // call to check_sol_is_correct so its lifetime is one solution
  // event.
  bool lastSolutionDominated = false;

  vector<vector<AnyVarRef>>& getPrintMatrix() {
    return print_matrix;
  }

  GenericBacktracker& getGenericBacktracker() {
    return generic_backtracker;
  }

  ProbSpec::CSPInstance* getInstance() {
    return csp_instance;
  }

  void setInstance(ProbSpec::CSPInstance* _csp) {
    csp_instance = _csp;
  }

  vector<set<AbstractConstraint*>>& getConstraintsToPropagate() {
    return constraintsToPropagate;
  }

  Controller::SearchManager* getSearchManager() {
    return currentSearchManager;
  }

  void setSearchManager(Controller::SearchManager* sm) {
    currentSearchManager = sm;
  }

  long long getNodeCount() {
    return nodes;
  }
  long long getBacktrackCount() {
    return backtracks;
  }
  long long getParallelPreprocessRounds() {
    return parallelPreprocessRounds;
  }
  void incrementParallelPreprocessRounds(long long n = 1) {
    parallelPreprocessRounds += n;
  }
  long long getParallelPreprocessPrunings() {
    return parallelPreprocessPrunings;
  }
  void incrementParallelPreprocessPrunings(long long n = 1) {
    parallelPreprocessPrunings += n;
  }
  void setNodeCount(long long _nodes) {
    nodes = _nodes;
  }
  void incrementNodeCount(long long n = 1) {
    nodes += n;
  }
  void incrementBacktrackCount() {
    backtracks++;
  }

  void resetSearchCounters() {
    nodes = 0;
    backtracks = 0;
    solutions = 0;
  }

  vector<AnyVarRef>& getOptimiseVars() {
    return optimiseVars;
  }
  void setOptimiseVars(const vector<AnyVarRef>& _var) {
    optimiseVars = _var;
  }

  vector<AnyVarRef>& getRawOptimiseVars() {
    return raw_optimiseVars;
  }
  void setRawOptimiseVars(const vector<AnyVarRef>& _var) {
    raw_optimiseVars = _var;
  }

  const vector<DomainInt>& getOptimiseValues() {
    return current_optimise_positions;
  }

  void setOptimiseValue(const vector<DomainInt>& optimise_pos) {
    current_optimise_positions = optimise_pos;
  }

  const vector<DomainInt>& getLastOptimumValues() {
    return last_optimum_values;
  }

  void setLastOptimumValues(const vector<DomainInt>& vals) {
    last_optimum_values = vals;
  }

  bool isOptimisationProblem() {
    return optimise;
  }
  void setOptimisationProblem(bool _optimise) {
    optimise = _optimise;
  }

  bool isMaximise() {
    return maximise;
  }
  void setMaximise(bool _maximise) {
    maximise = _maximise;
  }

  bool addConstraint(AbstractConstraint* c);
  const vector<AbstractConstraint*>& getConstraintList() {
    return constraints;
  }

  bool addConstraintMidsearch(AbstractConstraint* c);

  long long int getSolutionCount() {
    return solutions;
  }

  void setSolutionCount(long long int _sol) {
    solutions = _sol;
  }
  void incrementSolutionCount(long long int inc = 1) {
    solutions += inc;
  }

  bool isFinished() {
    return finished;
  }
  void setFinished(bool b) {
    finished = b;
  }

  bool isFailed() {
    return failed;
  }

  void setFailed() {
    failed = true;
  }

  // This should only be called by 'pop_world', or with
  // extreme care.
  void _unsafeClearFailed() {
    failed = false;
  }

  void setFailed(bool f) {
    failed = f;
  }
  // This function is here because a number of pieces of code want a raw
  // reference to the 'failed' variable.
  // Long term, this may get removed, but it is added for now to minimise
  // changes while removing global
  // variables.
  bool* getFailedPtr() {
    return &failed;
  }

  TimerClass& getOldTimer() {
    return oldtimer;
  }

  TupleListContainer* getTupleListContainer() {
    return &*tupleListContainer;
  }
  ShortTupleListContainer* getShortTupleListContainer() {
    return &*shortTupleListContainer;
  }

  void setTupleListContainer(shared_ptr<TupleListContainer> _tupleList) {
    tupleListContainer = _tupleList;
  }

  void setShortTupleListContainer(shared_ptr<ShortTupleListContainer> _tupleList) {
    shortTupleListContainer = _tupleList;
  }

  SearchState()
      : nodes(0),
        backtracks(0),
        optimise(false),
        constraintsToPropagate(1),
        solutions(0),
        finished(false),
        failed(false)
        {}

  // Must be defined later.
  ~SearchState();
};

struct NhConfig;
std::shared_ptr<NhConfig> makeNhConfig();

/// Stored all the options related to search. This item should not
/// be changed during search.
class SearchOptions {

public:
  struct RestartStruct {
    bool active = false;
    double multiplier = 1.5;
    bool bias = true;
  };

  RestartStruct restart;

  /// Denotes if minion should print no output, other than that explicitally
  /// requested
  bool silent;

  /// Denotes if minion prints only the optimal solution for optimisation
  /// problems.
  bool printonlyoptimal;

  /// Denotes if the search tree should be printed.
  bool dumptree;
  /// Store the current search tree (if a non-zero pointer)
  std::shared_ptr<SearchDumper> dumptreeobj;
  /// Gives the solutions which should be found.
  /// -1 denotes finding all solutions.
  long long sollimit;
  /// Denotes if solutions should be checked it they satisfy constraints.
  /// Only for debugging.
  bool nocheck;
  /// Denotes to nodelimit, 0 if none given.
  long long nodelimit;
  /// Denotes if information about search should be printed to a file.
  bool tableout;

  /// Denotes if solutions should be printed to a seperate file.
  bool solsoutWrite;

  /// Denotes if solutions should be written to seperate file in JSON
  bool solsoutJson = false;

  /// Denotes if solutions should be printed.
  /// Initialised to true.
  bool print_solution;

  /// Is there a timeout?
  bool timeoutActive;

  /// Stores the timelimit.
  clock_t time_limit;

  /// Stores if the timelimit is CPU time (yes) or wall-clock time (no)
  bool time_limit_is_CPUTime;

  /// Denotes if the variable and value orderings should be randomised.
  /// Initialised to false.
  bool randomiseValvarorder;

  /// Denotes if parser should output verbosely
  bool parserVerbose;

  /// The filename of the current input file (-- if reading from command line)
  string instance_name;

  bool redump;
  bool instance_stats;

  // Do not write a resume file.
  bool noresumefile;

  // split search tree in half on time out
  bool split;

  bool splitstderr;

  // files containing list of commands for minion to run
  std::string commandlistIn;
  std::string commandlistOut;

  /// Path of the file -solsout / -jsonsolsout opened (empty if neither
  /// was given). Recorded so worker-context library entrypoints can
  /// re-open the same file for their own per-thread ofstream — needed
  /// under the experimental thread-mode flags where each worker has
  /// its own globals->solsoutfile.
  string solsoutFilename;

  /// Output a compressed file
  string outputCompressed;

  /// output a compressed list of domains
  bool outputCompressedDomains;


  bool parallel = false;
  int parallelcores = 0;
  bool parallelStealHigh = true;

  /// Number of worker processes for parallel SAC/SACBounds preprocessing.
  /// 0 means sequential (today's behaviour). Set via the experimental
  /// -X-parallelPreprocess [N] flag.
  int parallelPreprocessCores = 0;

  /// Number of OS threads for portfolio search. 0 means sequential (today's
  /// behaviour). Set via the experimental -X-parallelThreads [N] flag.
  /// Mutually exclusive with -parallel and -X-parallelPreprocess.
  int numParallelThreads = 0;
  /// Optional pointer to a `std::atomic<long long>` shared by all
  /// workers in a parallel run, used to broadcast the tightest
  /// optimisation bound seen by any worker so others can prune
  /// further branches without waiting to find a solution of their
  /// own. Stored as void* to avoid pulling <atomic> into solver.h.
  ///
  /// Direction: minion internally maximises OptimiseVars (negating
  /// user-side minimisation in optimiseMinimiseVars), so "tighter"
  /// bound = LARGER. The atomic stores the running max across all
  /// workers' post-bump optVals[0]; workers apply it as
  /// `vars[0].setMin(shared)` whenever shared exceeds their own
  /// current bound.
  ///
  /// Single-objective only — multi-objective lex optimisation needs
  /// a vector of bounds, which a single atomic can't hold. Workers
  /// skip the broadcast when optVals.size() != 1.
  ///
  /// Set by runMinionParallel and runMinionWorkSteal before workers
  /// start; nullptr when no parallel run is active. The sentinel
  /// "no bound seen yet" value is `LLONG_MIN`.
  void* parallelBoundChannel = nullptr;

  /// Number of OS threads for work-stealing search. 0 means sequential.
  /// Set via the experimental -X-parallelWorkSteal [N] flag. Each worker
  /// has its own context but they cooperatively split the search tree
  /// via path-replay donation. Mutually exclusive with all other parallel
  /// modes.
  int numWorkStealThreads = 0;
  /// Portfolio mode for work-stealing: each worker uses a different
  /// (varorder, valorder, randomiseValvarorder) combination. Worker 0
  /// keeps the user's chosen heuristic; workers >= 1 cycle through a
  /// palette. Donation/replay is heuristic-independent so different
  /// strategies cooperate on the same shared tree.
  bool parallelWorkStealPortfolio = false;
  /// Pointer to the WorkStealController for this run; set by the
  /// orchestrator before workers start, accessed from inside the search
  /// loop's donation poll. Type-erased here to avoid pulling
  /// work_steal.h into solver.h.
  void* workStealController = nullptr;
  /// Worker index when this context is a work-stealing worker (>= 0) or
  /// -1 when it is not. Worker 0 bootstraps at the search root; workers
  /// >= 1 start by waiting on the shared queue.
  int workStealWorkerIdx = -1;

  // Gather AMOs
  bool gatherAMOs = false;
  bool gatherAMOsExtra = false;
  // Special search procedure for tabulation -- i.e. generating a table constraint.
  bool tabulationMode = false;

  // How (if at all) to autogenerate short tuples from long ones.
  MapLongTuplesToShort map_long_short;

  bool ensureBranchOnAllVars;

  SearchOptions()
      : silent(false),
        printonlyoptimal(false),
        dumptree(false),
        sollimit(1),
#ifdef NO_DEBUG
        nocheck(true),
#else
        nocheck(false),
#endif
        nodelimit(std::numeric_limits<long long>::max()),
        tableout(false),
        solsoutWrite(false),
        print_solution(true),
        timeoutActive(false),
        time_limit(0),
        time_limit_is_CPUTime(false),
        randomiseValvarorder(false),
        parserVerbose(false),
        redump(false),
        instance_stats(false),
        noresumefile(true),
        split(false),
        outputCompressedDomains(false),
        map_long_short(MLTTS_NoMap),
        ensureBranchOnAllVars(true) {
  }

  /// Denotes all solutions should be found, by setting sollimit to -1.
  void findAllSolutions() {
    sollimit = -1;
  }

  void print(string s) {
    if(!silent)
      getOutput() << s;
  }

  void printLine(string s) {
    if(!silent)
      getOutput() << s << endl;
  }
};

namespace Controller {

/// Pushes the state of the whole world.
inline void worldPush();

/// Pops the state of the whole world.
inline void worldPop();

inline void worldPop_all();
} // namespace Controller

#endif
