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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _SOLVER_H
#define _SOLVER_H

#include "system/system.h"

#include "tuple_container.h"

#include "StateObj_forward.h"

// Some advanced definitions, we don't actually need to know anything about these
// types for SearchState, simply that they exist.
class AbstractConstraint;
class AnyVarRef;



namespace ProbSpec {
  class CSPInstance;
}

class SearchState
{
  StateObj* stateObj;
  unsigned long long nodes;
  AnyVarRef* optimise_var;
  DomainInt current_optimise_position;
  bool optimise;
  
  // The variables to print when a solution is found.
  vector<vector<AnyVarRef> > print_matrix;
  
  vector<AbstractConstraint*> constraints;
  
  vector<set<AbstractConstraint*> > constraints_to_propagate;
  
#ifdef DYNAMICTRIGGERS
  vector<AbstractConstraint*> dynamic_constraints;
#endif
  
  long long int solutions;
  
  bool dynamic_triggers_used;
  
  bool finished;
  bool failed;
  jmp_buf g_env;

  ProbSpec::CSPInstance* csp_instance;

  
  TimerClass oldtimer;
  
  shared_ptr<TupleListContainer> tupleListContainer;

  bool is_locked;
  
  volatile bool alarm_trigger;
  
  volatile bool ctrl_c_pressed;
    
public:

  vector<vector<AnyVarRef> >& getPrintMatrix()
  { return print_matrix; }
  
  ProbSpec::CSPInstance* getInstance() { return csp_instance; }
   
  void setInstance(ProbSpec::CSPInstance* _csp) { csp_instance = _csp; }

  vector<set<AbstractConstraint*> >& getConstraintsToPropagate()
  { return constraints_to_propagate; }
  
  unsigned long long getNodeCount() { return nodes; }
  void setNodeCount(unsigned long long _nodes) { nodes = _nodes; }
  void incrementNodeCount() { nodes++; }
  
  AnyVarRef* getOptimiseVar() { return optimise_var; }
  void setOptimiseVar(AnyVarRef* _var) { optimise_var = _var; }
  
  DomainInt getOptimiseValue() { return current_optimise_position; }
  void setOptimiseValue(DomainInt optimise_pos) { current_optimise_position = optimise_pos; }
  
  bool isOptimisationProblem() { return optimise; }
  void setOptimisationProblem(bool _optimise) { optimise = _optimise; }
  
  void addConstraint(AbstractConstraint* c);
  vector<AbstractConstraint*>& getConstraintList() { return constraints; }
  
  void addConstraintMidsearch(AbstractConstraint* c);
  void redoFullPropagate(AbstractConstraint* c);
  
  long long int getSolutionCount() { return solutions; }
  void setSolutionCount(long long int _sol) { solutions = _sol; }
  void incrementSolutionCount() { solutions++; }
  
  bool isDynamicTriggersUsed() { return dynamic_triggers_used; }
  void setDynamicTriggersUsed(bool b) { dynamic_triggers_used = b; }
  
  bool isFinished() { return finished; }
  void setFinished(bool b) { finished = b; }
  
  bool isFailed() { return failed; }
  void setFailed(bool f) {
    failed = f; 
  }
  // This function is here because a number of pieces of code want a raw reference to the 'failed' variable.
  // Long term, this may get removed, but it is added for now to minimise changes while removing global
  // variables.
  bool* getFailedPtr() { return &failed; }
  
  TimerClass& getOldTimer() { return oldtimer; }
  
  
  jmp_buf* getJmpBufPtr() { return &g_env; }
  
  TupleListContainer* getTupleListContainer() { return &*tupleListContainer; }
  
  void setTupleListContainer(shared_ptr<TupleListContainer> _tupleList) 
  { tupleListContainer = _tupleList; }
                          
  SearchState(StateObj* _stateObj) : stateObj(_stateObj), nodes(0), optimise_var(NULL), 
    current_optimise_position(0), optimise(false), constraints_to_propagate(1),
    solutions(0), dynamic_triggers_used(false), finished(false), failed(false), 
    is_locked(false), alarm_trigger(false), ctrl_c_pressed(false)
  {}
  
  // Must be defined later.
  ~SearchState();
  
  void markLocked()
  { is_locked = true; }

  bool isLocked()
  { return is_locked; }
  
  bool isAlarmActivated()
  { return alarm_trigger; }

  void clearAlarm()
  { alarm_trigger = false; }  
  
  void setupAlarm(int timeout, bool CPU_time)
  { activate_trigger(&alarm_trigger, timeout, CPU_time);}
  
  bool isCtrlcPressed()
  { return ctrl_c_pressed; }
  
  void setupCtrlc()
  { install_ctrlc_trigger(&ctrl_c_pressed); }
  
  
};

/// Stored all the options related to search. This item should not
/// be changed during search.
class SearchOptions
{
public:
  /// Denotes if wdeg is turned on
  bool wdeg_on;

  /// Denotes if only generators for group should be found (only makes sense for groups)
  bool find_generators;

  /// Denotes if we should output in a compatable way to the solver competition.
  bool cspcomp;
  
  /// Denotes if minion should print no output, other than that explicitally requested
  bool silent;
  
  /// Denotes if the search tree should be printed.
  bool dumptree;
  /// Gives the solutions which should be found. 
  /// -1 denotes finding all solutions.
  long long sollimit;
  /// Denotes if non-incremental propagation should be used. 
  /// Only for debugging.
  bool fullpropagate;
  /// Denotes if solutions should be checked it they satisfy constraints.
  /// Only for debugging.
  bool nocheck;
  /// Denotes to nodelimit, 0 if none given.
  unsigned long long nodelimit;
  /// Denotes if information about search should be printed to a file.
  bool tableout;
  
  /// Denotes if solutions should be printed to a seperate file.
  bool solsoutWrite;

  /// A callback function for when a solution is found.
  boost::function< void (StateObj*)> solCallBack;

  /// Denotes if solutions should be printed.
  /// Initialised to true.
  bool print_solution;
    
  /// Stores the timelimit, 0 if none given.
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

  //do we resume from a previous run and, if so, what file
  bool resume;
  string resume_file;

  //doing restarts?
  bool doing_restarts;

  //learn cons after restarts?
  bool learn_restart_cons;

  // The format of output used (-1 for default)
  int outputType;
  
  /// Disable the use of linux timers
  bool noTimers;
  
  SearchOptions() : 
    wdeg_on(false), find_generators(false), 
    cspcomp(false), silent(false), dumptree(false), sollimit(1), fullpropagate(false), 
#ifdef NO_DEBUG
    nocheck(true),
#else
    nocheck(false),
#endif
    nodelimit(0), tableout(false), solsoutWrite(false), 
    print_solution(true), time_limit(0), time_limit_is_CPU_time(false),
    randomise_valvarorder(false), parser_verbose(false), 
    redump(false), graph(false), doing_restarts(false), learn_restart_cons(true), 
    outputType(-1), noTimers(false)
  {}
  
  /// Denotes all solutions should be found, by setting sollimit to -1.
  void findAllSolutions()
  { sollimit = -1; }
  
  void print(string s)
  {
    if(!silent)
     cout << s; 
  }
  
  void printLine(string s)
  { 
    if(!silent)
    cout << s << endl; 
  }
};

namespace Controller
{
  void lock(StateObj*);
  
  /// Pushes the state of the whole world.
  inline void world_push(StateObj* stateObj);

  /// Pops the state of the whole world.
  inline void world_pop(StateObj* stateObj);

  inline void world_pop_all(StateObj* stateObj);
}

#endif
