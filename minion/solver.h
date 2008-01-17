/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/* Minion
* Copyright (C) 2006
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


// Some advanced definitions, we don't actually need to know anything about these
// types for SearchState, simply that they exist.
class Constraint;
class DynamicConstraint;
class AnyVarRef;

class SearchState
{
  unsigned long long nodes;
  AnyVarRef* optimise_var;
  DomainInt current_optimise_position;
  bool optimise;
  
  vector<Constraint*> constraints;
#ifdef DYNAMICTRIGGERS
  vector<DynamicConstraint*> dynamic_constraints;
#endif
  
  long long int solutions;
  
  bool dynamic_triggers_used;
  
  bool finished;
  bool failed;
  jmp_buf g_env;
public:
	
	unsigned long long getNodeCount() { return nodes; }
  void setNodeCount(unsigned long long _nodes) { nodes = _nodes; }
  void incrementNodeCount() { nodes++; }
  
  AnyVarRef* getOptimiseVar() { return optimise_var; }
  void setOptimiseVar(AnyVarRef* _var) { optimise_var = _var; }
  
  DomainInt getOptimiseValue() { return current_optimise_position; }
  void setOptimiseValue(DomainInt optimise_pos) { current_optimise_position = optimise_pos; }
  
  bool isOptimisationProblem() { return optimise; }
  void setOptimisationProblem(bool _optimise) { optimise = _optimise; }
  
  void addConstraint(Constraint* c) { constraints.push_back(c); }
  vector<Constraint*>& getConstraintList() { return constraints; }
#ifdef DYNAMICTRIGGERS
  void addDynamicConstraint(DynamicConstraint* c) { dynamic_constraints.push_back(c); }
  vector<DynamicConstraint*>& getDynamicConstraintList() { return dynamic_constraints; }
#endif
  
  long long int getSolutionCount() { return solutions; }
  void setSolutionCount(long long int _sol) { solutions = _sol; }
  void incrementSolutionCount() { solutions++; }
  
  bool isDynamicTriggersUsed() { return dynamic_triggers_used; }
  void setDynamicTriggersUsed(bool b) { dynamic_triggers_used = b; }
  
  bool isFinished() { return finished; }
  bool setFinished(bool b) { finished = b; }
  
  bool isFailed() { return failed; }
  bool setFailed(bool f) { failed = f; }
  // This function is here because a number of pieces of code want a raw reference to the 'failed' variable.
  // Long term, this may get removed, but it is added for now to minimise changes while removing global
  // variables.
  bool* getFailedPtr() { return &failed; }
  
  jmp_buf* getJmpBufPtr() { return &g_env; }
  SearchState() : nodes(0), optimise_var(NULL), current_optimise_position(0), optimise(false), solutions(0),
	dynamic_triggers_used(false)
  {}
  
};

VARDEF(SearchState* state);

class SearchOptions
{
public:
  
  bool print_only_solution;
  bool dumptree;
  int sollimit;
  bool fullpropagate;
  bool nocheck;
  unsigned long long nodelimit;
  bool tableout;
  
  SearchOptions() : print_only_solution(false), dumptree(false), sollimit(-1), fullpropagate(false), 
	nocheck(false), nodelimit(0), tableout(false)
  {}
};

VARDEF(SearchOptions* options);


namespace Controller
{
  
  /// Called when search is finished. 
  /// This is mainly here so that any debugging instructions which watch for memory problems
  /// don't trigger when search is finished and memory is being cleaned up.
  inline void finish()
  { 
    D_INFO(0,DI_SOLVER,"Cleanup starts");
    state->setFinished(true);
  }
  

  
  /// Called whenever search fails.
  /// Anyone can call this at any time. Once the current propagator is left, search will backtrack.
  inline void fail()
  { 
    D_INFO(1,DI_SOLVER,"Failed!");
#ifdef USE_SETJMP
   SYSTEM_LONGJMP(*(state->getJmpBufPtr()),1);
#else
   state->setFailed(true);
#endif
  }
  
  void lock();
}

