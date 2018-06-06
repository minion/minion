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

#ifndef _SOLVER_H
#define _SOLVER_H

#include "system/system.h"

#include "tuple_container.h"

#include "StateObj_forward.h"

#include "memory_management/GenericBacktracker.h"

// Some advanced definitions, we don't actually need to know anything about
// these
// types for SearchState, simply that they exist.
class AbstractConstraint;
class AnyVarRef;

namespace ProbSpec {
struct CSPInstance;
}

class SearchState {

  long long nodes;
  long long backtracks;
  AnyVarRef* optimise_var;
  AnyVarRef* raw_optimise_var;
  DomainInt current_optimise_position;
  bool optimise;
  bool maximise;

  // The variables to print when a solution is found.
  vector<vector<AnyVarRef>> print_matrix;

  vector<AbstractConstraint*> constraints;

  vector<set<AbstractConstraint*>> constraints_to_propagate;

  long long int solutions;

  bool finished;
  bool failed;

  ProbSpec::CSPInstance* csp_instance;

  TimerClass oldtimer;

  shared_ptr<TupleListContainer> tupleListContainer;
  shared_ptr<ShortTupleListContainer> shortTupleListContainer;

  bool is_locked;

  volatile bool alarm_trigger;

  volatile bool ctrl_c_pressed;

  GenericBacktracker generic_backtracker;

public:
  std::string storedSolution;

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
    return constraints_to_propagate;
  }

  long long getNodeCount() {
    return nodes;
  }
  long long getBacktrackCount() {
    return backtracks;
  }
  void setNodeCount(long long _nodes) {
    nodes = _nodes;
  }
  void incrementNodeCount() {
    nodes++;
  }
  void incrementBacktrackCount() {
    backtracks++;
  }

  AnyVarRef* getOptimiseVar() {
    return optimise_var;
  }
  void setOptimiseVar(AnyVarRef* _var) {
    optimise_var = _var;
  }

  AnyVarRef* getRawOptimiseVar() {
    return raw_optimise_var;
  }
  void setRawOptimiseVar(AnyVarRef* _var) {
    raw_optimise_var = _var;
  }

  DomainInt getOptimiseValue() {
    return current_optimise_position;
  }
  void setOptimiseValue(DomainInt optimise_pos) {
    current_optimise_position = optimise_pos;
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

  void addConstraint(AbstractConstraint* c);
  vector<AbstractConstraint*>& getConstraintList() {
    return constraints;
  }

  void addConstraintMidsearch(AbstractConstraint* c);
  void redoFullPropagate(AbstractConstraint* c);

  long long int getSolutionCount() {
    return solutions;
  }
  void setSolutionCount(long long int _sol) {
    solutions = _sol;
  }
  void incrementSolutionCount() {
    solutions++;
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
        optimise_var(NULL),
        raw_optimise_var(NULL),
        current_optimise_position(0),
        optimise(false),
        constraints_to_propagate(1),
        solutions(0),
        finished(false),
        failed(false),
        is_locked(false),
        alarm_trigger(false),
        ctrl_c_pressed(false) {}

  // Must be defined later.
  ~SearchState();

  void markLocked() {
    is_locked = true;
  }

  bool isLocked() {
    return is_locked;
  }

  bool isAlarmActivated() {
    return alarm_trigger;
  }

  void clearAlarm() {
    alarm_trigger = false;
  }

  void setupAlarm(bool alarm_active, SysInt timeout, bool CPU_time) {
    activate_trigger(&alarm_trigger, alarm_active, timeout, CPU_time);
  }

  bool isCtrlcPressed() {
    return ctrl_c_pressed;
  }

  void setupCtrlc() {
    install_ctrlc_trigger(&ctrl_c_pressed);
  }
};

/// Stored all the options related to search. This item should not
/// be changed during search.
class SearchOptions {

public:
  struct NHConfig {
    // values here are not necessarily optimal (tuned)
    // they are defaults until tuning info can be given
    // can be overridden by commandline

    bool backtrackInsteadOfTimeLimit = true;
    int iterationSearchTime = 500;
    double initialSearchBacktrackLimitMultiplier = 2.0; //NGUYEN: test (2.0 is the best one return by tuning-initialise-only), original value: 1.5
    int initialBacktrackLimit = 22;
    double hillClimberBacktrackLimitMultiplier = 1.1;
    double hillClimberBacktrackLimitIncrement = 0;
    int lahcQueueSize = 100;
    double lahcStoppingLimitRatio = 1.0;
    double holePuncherBacktrackLimitMultiplier = 1.1;
    bool hillClimberIncreaseBacktrackOnlyOnFailure = true;
    int hillClimberMinIterationsToSpendAtPeak = 4;
    double hillClimberInitialLocalMaxProbability = 0.001;
    double hillClimberProbabilityIncrementMultiplier = 1.0 / 16;
    double ucbExplorationBias = 2;
    double learningAutomatonRate = 0.1;
    int holePuncherSolutionBagSizeConstant = 5;
    NHConfig() {}
  };

  enum class NeighbourhoodSearchStrategy {
    META_WITH_HILLCLIMBING,
    META_WITH_LAHC,
    HILL_CLIMBING,
    LAHC
  };
  enum class NeighbourhoodSelectionStrategy { RANDOM, UCB, LEARNING_AUTOMATON, INTERACTIVE };
  NeighbourhoodSearchStrategy neighbourhoodSearchStrategy =
      NeighbourhoodSearchStrategy::META_WITH_HILLCLIMBING;
  NeighbourhoodSelectionStrategy neighbourhoodSelectionStrategy =
      NeighbourhoodSelectionStrategy::UCB;
  NHConfig nhConfig;
  std::string pathToUCBInitFile;

  bool restarts = false;

  /// Denotes if minion should print no output, other than that explicitally
  /// requested
  bool silent;

  /// Denotes if minion prints only the optimal solution for optimisation
  /// problems.
  bool printonlyoptimal;

  /// Denotes if the search tree should be printed.
  bool dumptree;
  /// Store the current json search tree (and isActive if we should
  /// output it
  JSONStreamer dumptreejson;
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

  /// Denotes if solutions should be printed.
  /// Initialised to true.
  bool print_solution;

  /// Is there a timeout?
  bool timeout_active;

  /// Stores the timelimit.
  clock_t time_limit;

  /// Stores if the timelimit is CPU time (yes) or wall-clock time (no)
  bool time_limit_is_CPU_time;

  /// Denotes if the variable and value orderings should be randomised.
  /// Initialised to false.
  bool randomise_valvarorder;

  /// Denotes if parser should output verbosely
  bool parser_verbose;

  /// The filename of the current input file (-- if reading from command line)
  string instance_name;

  bool redump;
  bool graph;
  bool instance_stats;

  // Do not write a resume file.
  bool noresumefile;

  // split search tree in half on time out
  bool split;

  bool splitstderr;

  /// Output a compressed file
  string outputCompressed;

  /// output a compressed list of domains
  bool outputCompressedDomains;

  /// Disable the use of linux timers
  bool noTimers;

  string gapname;

  // How (if at all) to autogenerate short tuples from long ones.
  MapLongTuplesToShort map_long_short;

  bool ensure_branch_on_all_vars;

  SearchOptions()
      : silent(false),
        printonlyoptimal(false),
        dumptree(false),
        dumptreejson(),
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
        timeout_active(false),
        time_limit(0),
        time_limit_is_CPU_time(false),
        randomise_valvarorder(false),
        parser_verbose(false),
        redump(false),
        graph(false),
        instance_stats(false),
        noresumefile(true),
        split(false),
        outputCompressedDomains(false),
        noTimers(false),
        gapname("gap.sh"),
        map_long_short(MLTTS_NoMap),
        ensure_branch_on_all_vars(true) {
  }

  /// Denotes all solutions should be found, by setting sollimit to -1.
  void findAllSolutions() {
    sollimit = -1;
  }

  void print(string s) {
    if(!silent)
      cout << s;
  }

  void printLine(string s) {
    if(!silent)
      cout << s << endl;
  }
};

namespace Controller {
void lock();

/// Pushes the state of the whole world.
inline void world_push();

/// Pops the state of the whole world.
inline void world_pop();

inline void world_pop_all();
} // namespace Controller

#endif
