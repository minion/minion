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

#include "search_dump.hpp"

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
  vector<AnyVarRef> optimiseVars;
  vector<AnyVarRef> raw_optimiseVars;
  vector<DomainInt> current_optimise_positions;
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

  volatile bool alarmTrigger;

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
    return constraintsToPropagate;
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
        failed(false),
        alarmTrigger(false) {}

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
  bool graph;
  bool instance_stats;

  // Do not write a resume file.
  bool noresumefile;

  // split search tree in half on time out
  bool split;

  bool splitstderr;

  // files containing list of commands for minion to run
  std::string commandlistIn;
  std::string commandlistOut;
  
  /// Output a compressed file
  string outputCompressed;

  /// output a compressed list of domains
  bool outputCompressedDomains;

  string gapname;

  bool parallel = false;
  int parallelcores = 0;
  bool parallelStealHigh = true;

  // Gather AMOs
  bool gatherAMOs = false;

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
        graph(false),
        instance_stats(false),
        noresumefile(true),
        split(false),
        outputCompressedDomains(false),
        gapname("gap.sh"),
        map_long_short(MLTTS_NoMap),
        ensureBranchOnAllVars(true) {
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
inline void worldPush();

/// Pops the state of the whole world.
inline void worldPop();

inline void worldPop_all();
} // namespace Controller

#endif
