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

#include <cstdlib>

#ifndef ABSTRACT_CONSTRAINT_H
#define ABSTRACT_CONSTRAINT_H

#include "../system/system.h"
#include "../solver.h"
#include "../variables/AnyVarRef.h"
#include <vector>

using namespace std;

class AnyVarRef;
class DynamicTrigger;

#include "dynamic_trigger.h"

struct AbstractTriggerCreator;
typedef vector<shared_ptr<AbstractTriggerCreator> > triggerCollection;

#include "constraint_printing.h"


/// Base type from which all constraints are derived.
class AbstractConstraint
{
protected:
  /// Private members of the base class.

  
  void* _DynamicTriggerCache;
  vector<AnyVarRef> singleton_vars;


public:

  #ifdef WDEG
  UnsignedSysInt wdeg;
  #endif

  BOOL full_propagate_done;

  virtual string full_output_name()
  { D_FATAL_ERROR("Unimplemented output in " + extended_name()); }


  /// Returns a point to the first dynamic trigger of the constraint.
  DynamicTrigger* dynamic_trigger_start()
    { return static_cast<DynamicTrigger*>(_DynamicTriggerCache); }

  /// Defines the number of dynamic triggers the constraint wants.
  /// Must be implemented by any constraint.
  virtual SysInt dynamic_trigger_count()
    { return 0; }

  /// Returns the number of dynamic triggers for this constraint, and all the children of
  /// this constraint. Only differs from the above for things like 'reify', 'reifyimply' and 'or'.
  virtual SysInt dynamic_trigger_count_with_children()
    { return dynamic_trigger_count(); }

  /// Gets all the triggers a constraint wants to set up.
  /** This function shouldn't do any propagation. That is full_propagate's job.*/
  virtual triggerCollection setup_internal()
    { return triggerCollection(); }

  /// Iterative propagation function.
  /** Can assume full_propagate is always called at least once before propagate */
  virtual void propagate(DynamicTrigger*)
    { D_FATAL_ERROR("Fatal error in 'Dynamic Propagate' in " + extended_name()); }

  /// Iterative propagation function.
  /** Can assume full_propagate is always called at least once before propagate */
  virtual void propagate(DomainInt, DomainDelta)
    { D_FATAL_ERROR("Fatal error in 'Static Propagate' in " + extended_name()); }


  /// Looks for a valid partial assignment to a constraint.
  /** The return value (in the box) is pairs of <varnum, domain value>, where varnum is in the same position
  *  as returned by get_vars.
   */
    virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
    ostringstream oss;
    oss << "Finding assignment is not supported by the " << extended_name() << " constraint. Sorry" << endl;
    output_fatal_error(oss.str());
    return false;
  }

  /// Returns the reverse of the current constraint
  /** Used by rarification */
  virtual AbstractConstraint* reverse_constraint()
  {
    ostringstream oss;
    oss << "Negation is not supported by the " << extended_name() << " constraint. Sorry" << endl;
    output_fatal_error(oss.str());
    return NULL;
  }

  AbstractConstraint() :
    _DynamicTriggerCache(), singleton_vars(),
#ifdef WDEG
    wdeg(1),
#endif
    full_propagate_done(false)
    {}

  /// Method to get constraint name for output.
  virtual string constraint_name() = 0;

  /// Method to get constraint name for debugging.
  virtual string extended_name()
  { return constraint_name(); }


  /// Performs a full round of propagation and sets up any data needs by propagate().
  /** This function can be called during search if the function is reified */
  virtual void full_propagate() = 0;

  // Returns the variables of the constraint
  virtual vector<AnyVarRef> get_vars() = 0;

  vector<AnyVarRef>* get_vars_singleton() //piggyback singleton vector on get_vars()
  {
    if(singleton_vars.size() == 0) singleton_vars = get_vars(); //for efficiency: no constraint over 0 variables
    return &singleton_vars;
  }

#ifdef WDEG
  inline UnsignedSysInt getWdeg() { return wdeg; }

  inline void incWdeg()
  {
    wdeg += 1;
    vector<AnyVarRef>* vars = get_vars_singleton();
    size_t vars_s = vars->size();
    for(size_t i = 0; i < vars_s; i++)
      (*vars)[i].incWdeg();
  }
#endif

  /// Allows functions to activate a special kind of trigger, run only
  /// after the normal queue is empty.
  virtual void special_check()
  {
    output_fatal_error("Serious internal error");
  }

  // Called if failure occurs without actiating a special trigger, so the constraint can unlock.
  virtual void special_unlock()
  {
    output_fatal_error("Serious internal error");
  }

  /// Checks if an assignment is satisfied.
  /** This takes the variable order returned by, and is mainly only used by, get_table_constraint() */
  virtual BOOL check_assignment(DomainInt* v, SysInt v_size) = 0;

  virtual ~AbstractConstraint()
    { }



  virtual void setup_dynamic_triggers(void* DynamicTriggerPointer)
    { _DynamicTriggerCache = DynamicTriggerPointer; }


  virtual triggerCollection setup_internal_gather_triggers()
    { return setup_internal(); }

  /// Actually creates the dynamic triggers. Calls dynamic_trigger_count from function to get
  /// the number of triggers required.
  virtual void setup()
  {
    // Dynamic initialisation
    const SysInt trigs = checked_cast<SysInt>(dynamic_trigger_count());
    D_ASSERT(trigs >= 0);
    setup_dynamic_triggers( checked_malloc((sizeof(DynamicTrigger) * trigs)));

    DynamicTrigger* start = dynamic_trigger_start();
    for(SysInt i = 0 ; i < trigs; ++i)
      new (start+i) DynamicTrigger(this, i);

    // Static initialisation
    triggerCollection t = setup_internal_gather_triggers();
    for(triggerCollection::iterator it = t.begin(); it != t.end(); ++it)
    {
      (*it)->post_trigger();
    }
  }

  SysInt getTightnessEstimate()
  {
      // Make 1000 random tuples and see if they satisfy the constraint
      vector<AnyVarRef> vars=get_vars();
      DomainInt* t=new DomainInt[vars.size()];
      SysInt unsatcounter=0;
      srand(12345);
      for(SysInt i=0; i<1000; i++)
      {
          for(SysInt j=0; j<(SysInt)vars.size(); j++)
          {
              DomainInt dsize=vars[j].getInitialMax()-vars[j].getInitialMin()+1;
              t[j]=(rand()%checked_cast<SysInt>(dsize))+vars[j].getInitialMin();
          }
          if(!check_assignment(t, vars.size()))
          {
              unsatcounter++;
          }
      }
      delete[] t;
      return unsatcounter;   // return tightness i.e. #forbidden tuples out of 1000
  }

  SysInt getTightnessEstimateVarVal(const size_t var, const DomainInt val)
  {
      // Make 100 random tuples and see if they satisfy the constraint
      vector<AnyVarRef> vars=get_vars();
      DomainInt* t=new DomainInt[vars.size()];
      t[var] = val; //fix specified component
      SysInt unsatcounter=0;
      srand(12345);
      for(SysInt i=0; i<100; i++)
      {
          for(size_t j=0; j<vars.size(); j++)
          {
	    if(j != var) {
              DomainInt dsize=vars[j].getInitialMax()-vars[j].getInitialMin()+1;
              t[j]=(rand()%dsize)+vars[j].getInitialMin();
	    }
          }
          if(!check_assignment(t, vars.size()))
          {
              unsatcounter++;
          }
      }
      delete[] t;
      return unsatcounter;   // return tightness i.e. #forbidden tuples out of 100
  }
  
  template<typename Var>
  void moveTrigger(Var& v, DynamicTrigger* t, TrigType type, DomainInt pos = NoDomainValue , TrigOp op = TO_Default)
  { v.addDynamicTrigger(this, t, type, pos, op); }
  
  template<typename Var>
  void moveTriggerInt(Var& v, DomainInt t, TrigType type, DomainInt pos = NoDomainValue , TrigOp op = TO_Default)
  {
    DynamicTrigger* dt = static_cast<DynamicTrigger*>(_DynamicTriggerCache);
    v.addDynamicTrigger(this, dt + checked_cast<SysInt>(t), type, pos, op);
  }
  
  SysInt& triggerInfo(SysInt t)
  {
    DynamicTrigger* dt = static_cast<DynamicTrigger*>(_DynamicTriggerCache);
    return (dt+t)->trigger_info();
  }
  
};

/// Constraint from which other constraints can be inherited. Extends dynamicconstraint to allow children to be dynamic.
class ParentConstraint : public AbstractConstraint
{
protected:
  vector<AbstractConstraint*> child_constraints;
  // Maps a dynamic trigger to the constraint which it belongs to.
  // SysInt as they never change, and are always used to index arrays
  vector<SysInt> _dynamic_trigger_to_constraint;
  // Maps a static trigger to a pair { constraint, trigger for that constraint }
  vector< pair<DomainInt, DomainInt> > _static_trigger_to_constraint;
  // Maps variables to constraints
  vector<DomainInt> variable_to_constraint;
  // Gets start of each constraint
  vector<DomainInt> start_of_constraint;
public:

  pair<DomainInt, DomainInt> getChildStaticTrigger(DomainInt i)
    { return _static_trigger_to_constraint[checked_cast<SysInt>(i)]; }

  SysInt getChildDynamicTrigger(DynamicTrigger* ptr)
  {
    return _dynamic_trigger_to_constraint[ptr - dynamic_trigger_start()];
  }

  /// Gets all the triggers a constraint wants to set up.
  /** This function shouldn't do any propagation. That is full_propagate's job.*/
  virtual triggerCollection setup_internal_gather_triggers()
  {
    triggerCollection newTriggers;

    for(UnsignedSysInt i = 0; i < child_constraints.size(); i++)
    {
      triggerCollection childTrigs = child_constraints[i]->setup_internal_gather_triggers();
      for(UnsignedSysInt j = 0; j < childTrigs.size(); ++j)
      {
        // Record each original trigger value, then add the modified trigger to the collection.
        _static_trigger_to_constraint.push_back(make_pair(i, childTrigs[j]->trigger.info));
        // Need a '-1' on the next line, as C++ containers are indexed from 0!
        childTrigs[j]->trigger.info = _static_trigger_to_constraint.size() - 1;
        childTrigs[j]->trigger.constraint = this;
        newTriggers.push_back(childTrigs[j]);
      }
    }

    triggerCollection parentTrigs = setup_internal();
    for(UnsignedSysInt i = 0; i < parentTrigs.size(); ++i)
      D_ASSERT(parentTrigs[i]->trigger.info < 0);
    newTriggers.insert(newTriggers.end(), parentTrigs.begin(), parentTrigs.end());

    return newTriggers;
  }

  ParentConstraint(const vector<AbstractConstraint*> _children = vector<AbstractConstraint*>()) :
  child_constraints(_children)
  {
    SysInt var_count = 0;
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i)
    {
      start_of_constraint.push_back(var_count);
      SysInt con_size = child_constraints[i]->get_vars_singleton()->size();
      for(SysInt j = 0; j < con_size; ++j)
      {
        variable_to_constraint.push_back(i);
      }
      var_count += con_size;
    }
  }

  virtual SysInt dynamic_trigger_count_with_children()
  {
    SysInt trigger_count = dynamic_trigger_count();
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i)
      trigger_count += child_constraints[i]->dynamic_trigger_count_with_children();
    return trigger_count;
  }

  virtual void setup_dynamic_triggers(void* dynamicTriggerPointer)
  {
    _DynamicTriggerCache = dynamicTriggerPointer;

    SysInt current_trigger_count = dynamic_trigger_count();

    for(SysInt count = 0; count < current_trigger_count; ++count)
      _dynamic_trigger_to_constraint.push_back(child_constraints.size());

    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i)
    {
      // We need this check to ensure we don't try constructing a "start of trigger" block one off the
      // the end of memory array.
      if(current_trigger_count == dynamic_trigger_count_with_children())
        return;

      // Get start child's dynamic triggers.
      void* childPtr = (char*)dynamicTriggerPointer + checked_cast<SysInt>(current_trigger_count) * sizeof(DynamicTrigger);
      child_constraints[i]->setup_dynamic_triggers(childPtr);

      SysInt child_trig_count = child_constraints[i]->dynamic_trigger_count_with_children();

      for(SysInt count = current_trigger_count; count < current_trigger_count + child_trig_count; ++count)
        _dynamic_trigger_to_constraint.push_back(i);

      current_trigger_count += child_trig_count;
    }
  }

  /// Actually creates the dynamic triggers. Calls dynamic_trigger_count from function to get
  /// the number of triggers required.
  virtual void setup()
  {
    // Dynamic initialisation
    const SysInt all_trigs = checked_cast<SysInt>(dynamic_trigger_count_with_children());

    D_DATA(DomainInt trigs = dynamic_trigger_count());
    D_ASSERT(trigs >= 0);
    D_ASSERT(all_trigs >= trigs);

    void* trigMem = checked_malloc(sizeof(DynamicTrigger) * all_trigs);

    // Start by allocating triggers in the memory block
    DynamicTrigger* start = static_cast<DynamicTrigger*>(trigMem);
    for(SysInt i = 0 ; i < all_trigs; ++i)
      new (start+i) DynamicTrigger(this, i);

    setup_dynamic_triggers(trigMem);

    // Static initialisation
    triggerCollection t = setup_internal_gather_triggers();
    for(triggerCollection::iterator it = t.begin(); it != t.end(); ++it)
    {
      (*it)->post_trigger();
    }
  }

  virtual ~ParentConstraint()
  {
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i)
      delete child_constraints[i];
  }
};

inline void DynamicTrigger::propagate()
{
  D_ASSERT(sanity_check == 1234);
  constraint->propagate(this);
}

namespace ConOutput
{
  inline
  string print_vars(AbstractConstraint* const& c)
  { return c->full_output_name(); }
}

#endif
