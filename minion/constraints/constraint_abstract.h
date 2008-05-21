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

/** @help constraints Description
Minion supports many constraints and these are regularly being
improved and added to. In some cases multiple implementations of the
same constraints are provided and we would appreciate additional
feedback on their relative merits in your problem.

Minion does not support nesting of constraints, however this can be
achieved by auxiliary variables and reification.

Variables can be replaced by constants. You can find out more on
expressions for variables, vectors, etc. in the section on variables.
*/

/** @help constraints References 
help variables
*/

#ifndef ABSTRACT_CONSTRAINT
#define ABSTRACT_CONSTRAINT

#include "../system/system.h"
#include "../solver.h"
#include "../variables/AnyVarRef.h"
#include <vector>

using namespace std;

class AnyVarRef;
class DynamicTrigger;

#include "dynamic_trigger.h"

#define DYNAMIC_PROPAGATE_FUNCTION virtual void propagate

#ifdef  FUNCTIONPOINTER_TRIGGER
#define PROPAGATE_FUNCTION void propagate
#else
#define PROPAGATE_FUNCTION virtual void propagate
#endif

struct AbstractTriggerCreator;
typedef vector<shared_ptr<AbstractTriggerCreator> > triggerCollection;

/// Base type from which all constraints are derived.
class AbstractConstraint
{
 public:

  vector<AnyVarRef> vars;
  StateObj* stateObj;
  BOOL full_propagate_done;
#ifdef WDEG
  unsigned int wdeg;
#endif

  /// Private member of the base class.
  MemOffset _DynamicTriggerCache;
  
  /// Returns a point to the first dynamic trigger of the constraint.
  DynamicTrigger* dynamic_trigger_start()
  { return static_cast<DynamicTrigger*>(_DynamicTriggerCache.get_ptr()); }
  
  /// Gives the value of a specific dynamic trigger.
  int dynamic_trigger_num(DynamicTrigger* trig)
  { return trig - static_cast<DynamicTrigger*>(_DynamicTriggerCache.get_ptr()); }

  /// Defines the number of dynamic triggers the constraint wants.
  /// Must be implemented by any constraint.
  virtual int dynamic_trigger_count() 
    { return 0; }
  
  /// Gets all the triggers a constraint wants to set up.
  /** This function shouldn't do any propagation. That is full_propagate's job.*/
  virtual triggerCollection setup_internal()
    { return triggerCollection(); }
  
  /// Iterative propagation function.
  /** Can assume full_propagate is always called at least once before propagate */
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger*)
  { D_FATAL_ERROR("Fatal error in 'Dynamic Propagate' in " + constraint_name()); }
  
    /// Iterative propagation function.
    /** Can assume full_propagate is always called at least once before propagate */
  #ifdef FUNCTIONPOINTER_TRIGGER
    PROPAGATE_FUNCTION(int, DomainDelta) {}
  #else
    PROPAGATE_FUNCTION(int, DomainDelta) 
    { D_FATAL_ERROR("Fatal error in 'Static Propagate' in " + constraint_name()); }
  #endif
  
  /// Checks if a constraint cannot be satisfied, and sets up any data structures for future incremental checks.
  /// Returns TRUE if constraint cannot be satisfied.
  /** This function is used by rarification */
  virtual BOOL full_check_unsat()
  { 
	  cerr << "Static reification is not supported by the " << constraint_name() << " constraint. Sorry" << endl;
    exit(1); 
  }
    
  /// Checks incrementaly if constraint cannot be satisfied.
  /// Returns TRUE if constraint cannot be satisfied.
  /** This function should not be called unless check_unsat_full is called first. This is used by rarification */
  virtual BOOL check_unsat(int,DomainDelta)
  { 
	  cerr << "Static reification is not supported by the " << constraint_name() << " constraint. Sorry" << endl;
    exit(1); 
  }
    
  /// Returns the reverse of the current constraint
  /** Used by rarification */
  virtual AbstractConstraint* reverse_constraint()
  { 
	  cerr << "Static reification is not supported by the " << constraint_name() << " constraint. Sorry" << endl;
    exit(1);
  }
  
  AbstractConstraint(StateObj* _stateObj) : 
#ifdef WDEG
    wdeg(1),
#endif
    stateObj(_stateObj), full_propagate_done(false)
    {}

  /// Method to get constraint name for debugging.
  virtual string constraint_name() = 0;
  
  /// Performs a full round of propagation and sets up any data needs by propagate().
  /** This function can be called during search if the function is reified */
  virtual void full_propagate() = 0;
  
  // Returns the variables of the constraint
  virtual vector<AnyVarRef> get_vars() = 0;
  
  vector<AnyVarRef>* get_vars_singleton() //piggyback singleton vector on get_vars()
  { 
    if(vars.size() == 0) vars = get_vars(); //for efficiency: no constraint over 0 variables
    return &vars; 
  }

#ifdef WDEG
  unsigned int getWdeg();

  unsigned int incWdeg();
#endif

  /// Allows functions to activate a special kind of trigger, run only
  /// after the normal queue is empty.
  virtual void special_check()
  { 
	cerr << "Serious internal error" << endl;
	FAIL_EXIT(); 
  }
  
  virtual void special_unlock()
  { 
	cerr << "Serious internal error" << endl;
	FAIL_EXIT(); 
  }
  
  /// Checks if an assignment is satisfied.
  /** This takes the variable order returned by, and is mainly only used by, get_table_constraint() */
  virtual BOOL check_assignment(DomainInt* v, int v_size) = 0;
    
  virtual ~AbstractConstraint()
  {}
  
  /// Actually creates the dynamic triggers. Calls dynamic_trigger_count from function to get
  /// the number of triggers required.
  virtual void setup()
  {
    // Dynamic initialisation
    int trigs = dynamic_trigger_count();
    D_ASSERT(trigs >= 0);
    _DynamicTriggerCache = getMemory(stateObj).nonBackTrack().request_bytes((sizeof(DynamicTrigger) * trigs));
    
	DynamicTrigger* start = dynamic_trigger_start();
	for(int i = 0 ; i < trigs; ++i)
	  new (start+i) DynamicTrigger(this);
	  
	  // Static initialisation
	  triggerCollection t = setup_internal();
    for(triggerCollection::iterator it = t.begin(); it != t.end(); ++it)
    {
      (*it)->post_trigger();
    }
  }
  
  
};



#endif
