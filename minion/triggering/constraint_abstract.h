// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#include <cstdlib>

#ifndef ABSTRACT_CONSTRAINT_H
#define ABSTRACT_CONSTRAINT_H

#include "../system/system.h"

#include "../solver.h"

#include "../variables/AnyVarRef.h"
#include "../globals.h"

#include <vector>

using namespace std;

class AnyVarRef;

#include "dynamic_trigger.h"

#include "constraint_printing.h"

/// Base type from which all constraints are derived.
class AbstractConstraint {
protected:
  AbstractConstraint* parent;
  SysInt childpos;

  /// Private members of the base class.

  vector<Con_TrigRef> trig_infoVec;

  vector<AnyVarRef> singleton_vars;

  vector<SysInt> Trigger_info;

public:
  void _setParent(AbstractConstraint* _parent, SysInt _childpos) {
    D_ASSERT(parent == (AbstractConstraint*)BAD_POINTER);
    parent = _parent;
    childpos = _childpos;
  }

  Con_TrigRef _getTrigRef(SysInt trigger) {
    D_ASSERT(parent == nullptr);
    return trig_infoVec[trigger];
  }

  void _reportTriggerMovementToConstraint(SysInt trigger, Con_TrigRef tpi) {
    D_ASSERT(parent == nullptr);
    D_ASSERT(trigger >= 0 && trigger < trig_infoVec.size());
    TRIGP("CM" << trig_infoVec[trigger] << "->" << trigger << ":" << tpi);
    trig_infoVec[trigger] = tpi;
  }

  void _reportTriggerRemovalToConstraint(SysInt trigger) {
    D_ASSERT(parent == nullptr);
    D_ASSERT(trigger >= 0 && trigger < trig_infoVec.size());
    TRIGP("CR" << trig_infoVec[trigger] << ":" << trigger);
    trig_infoVec[trigger] = Con_TrigRef{};
  }

  virtual Trig_ConRef _parent_map_Trig_ConRef(SysInt child, SysInt trigger) {
    INTERNAL_ERROR("?");
  }

  virtual Con_TrigRef _parent_map_Con_TrigRef(SysInt child, SysInt trigger) {
    INTERNAL_ERROR("?");
  }

  void restoreTriggerOnBacktrack(SysInt trigger) {
    Con_TrigRef t;

    // XXX ?
    if(parent == nullptr) {
      t = parent->_parent_map_Con_TrigRef(childpos, trigger);
    } else {
      D_ASSERT(trigger >= 0 && trigger < trig_infoVec.size());
      t = trig_infoVec[trigger];
    }
    Trig_ConRef tcr = t.dtl->_getConRef(t.triggerListPos);

    _restoreTriggerOnBacktrack(tcr);
  }

#ifdef WDEG
  UnsignedSysInt wdeg;
#endif

  BOOL fullPropagateDone;

  virtual string fullOutputName() {
    D_FATAL_ERROR("Unimplemented output in " + extendedName());
  }

  /// Defines the number of dynamic triggers the constraint wants.
  /// Must be implemented by any constraint.
  virtual SysInt dynamicTriggerCount() {
    return 0;
  }

  /// Returns the number of dynamic triggers for this constraint, and all the
  /// children of
  /// this constraint. Only differs from the above for things like 'reify',
  /// 'reifyimply' and 'or'.
  virtual SysInt dynamicTriggerCountWithChildren() {
    return dynamicTriggerCount();
  }

  /// Iterative propagation function.
  /** Can assume fullPropagate is always called at least once before propagate
   */
  virtual void propagateDynInt(SysInt, DomainDelta) {
    D_FATAL_ERROR("Fatal error in 'Dynamic Propagate' in " + extendedName());
  }

  /// Looks for a valid partial assignment to a constraint.
  /** The return value (in the box) is pairs of <varnum, domain value>, where
   * varnum is in the same position
   *  as returned by getVars.
   */
  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    ostringstream oss;
    oss << "Finding assignment is not supported by the " << extendedName() << " constraint. Sorry"
        << endl;
    outputFatalError(oss.str());
    return false;
  }

  /// Returns the reverse of the current constraint
  /** Used by rarification */
  virtual AbstractConstraint* reverseConstraint() {
    ostringstream oss;
    oss << "Negation is not supported by the " << extendedName() << " constraint. Sorry" << endl;
    outputFatalError(oss.str());
    return NULL;
  }

  AbstractConstraint()
      : parent((AbstractConstraint*)BAD_POINTER),
        childpos(-1),
        singleton_vars(),
#ifdef WDEG
        wdeg(1),
#endif
        fullPropagateDone(false) {
  }

  /// Method to get constraint name for output.
  virtual string constraintName() = 0;

  /// Method to get constraint name for debugging.
  virtual string extendedName() {
    return constraintName();
  }

  /// Performs a full round of propagation and sets up any data needs by
  /// propagateDynInt().
  /** This function can be called during search if the function is reified */
  virtual void fullPropagate() = 0;

  // Returns the variables of the constraint
  virtual vector<AnyVarRef> getVars() = 0;

  vector<AnyVarRef>* getVarsSingleton() // piggyback singleton vector on getVars()
  {
    if(singleton_vars.size() == 0)
      singleton_vars = getVars(); // for efficiency: no constraint over 0 variables
    return &singleton_vars;
  }

#ifdef WDEG
  inline UnsignedSysInt getWdeg() {
    return wdeg;
  }

  inline void incWdeg() {
    wdeg += 1;
    vector<AnyVarRef>* vars = getVarsSingleton();
    size_t vars_s = vars->size();
    for(size_t i = 0; i < vars_s; i++)
      (*vars)[i].incWdeg();
  }
#endif

  /// Allows functions to activate a special kind of trigger, run only
  /// after the normal queue is empty.
  virtual void specialCheck() {
    outputFatalError("Serious internal error");
  }

  // Called if failure occurs without actiating a special trigger, so the
  // constraint can unlock.
  virtual void specialUnlock() {
    outputFatalError("Serious internal error");
  }

  /// Checks if an assignment is satisfied.
  /** This takes the variable order returned by, and is mainly only used by,
   * get_table_constraint() */
  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) = 0;

  virtual ~AbstractConstraint() {}

  virtual void setupDynamicTriggers() {
    trig_infoVec.resize(dynamicTriggerCount());
  }

  virtual void setupDynamicTriggerDatastructures() {}

  /// Actually creates the dynamic triggers. Calls dynamicTriggerCount from
  /// function to get
  /// the number of triggers required.
  virtual void setup() {
    D_ASSERT(parent == (AbstractConstraint*)BAD_POINTER);
    parent = NULL;
    // Dynamic initialisation
    const SysInt trigs = checked_cast<SysInt>(dynamicTriggerCount());
    (void)trigs;
    D_ASSERT(trigs >= 0);
    setupDynamicTriggers();
  }

  SysInt getTightnessEstimate() {
    // Make 1000 random tuples and see if they satisfy the constraint
    vector<AnyVarRef> vars = getVars();
    DomainInt* t = new DomainInt[vars.size()];
    SysInt unsatcounter = 0;
    for(SysInt i = 0; i < 1000; i++) {
      for(SysInt j = 0; j < (SysInt)vars.size(); j++) {
        DomainInt dsize = vars[j].initialMax() - vars[j].initialMin() + 1;
        uniform_int_distribution<int> dist(0, checked_cast<SysInt>(dsize));
        t[j] = dist(GET_GLOBAL(global_random_gen)) + vars[j].initialMin();
      }
      if(!checkAssignment(t, vars.size())) {
        unsatcounter++;
      }
    }
    delete[] t;
    return unsatcounter; // return tightness i.e. #forbidden tuples out of 1000
  }

  SysInt getTightnessEstimateVarVal(const size_t var, const DomainInt val) {
    // Make 100 random tuples and see if they satisfy the constraint
    vector<AnyVarRef> vars = getVars();
    DomainInt* t = new DomainInt[vars.size()];
    t[var] = val; // fix specified component
    SysInt unsatcounter = 0;
    for(SysInt i = 0; i < 100; i++) {
      for(size_t j = 0; j < vars.size(); j++) {
        if(j != var) {
          DomainInt dsize = vars[j].initialMax() - vars[j].initialMin() + 1;
          uniform_int_distribution<int> dist(0, checked_cast<SysInt>(dsize));
          t[j] = dist(GET_GLOBAL(global_random_gen)) + vars[j].initialMin();
        }
      }
      if(!checkAssignment(t, vars.size())) {
        unsatcounter++;
      }
    }
    delete[] t;
    return unsatcounter; // return tightness i.e. #forbidden tuples out of 100
  }

  template <typename Var>
  void moveTriggerInt(Var& v, DomainInt t, TrigType type, DomainInt pos = NoDomainValue,
                      TrigOp op = TO_Default) {
    D_ASSERT(t >= 0 && t < dynamicTriggerCountWithChildren());
    if(parent != nullptr) {
      Trig_ConRef trig = parent->_parent_map_Trig_ConRef(childpos, checked_cast<SysInt>(t));
      v.addDynamicTrigger(trig, type, pos, op);
    } else
      v.addDynamicTrigger(Trig_ConRef{this, checked_cast<SysInt>(t)}, type, pos, op);
  }

  SysInt& triggerInfo(DomainInt t) {
    if(Trigger_info.size() <= t)
      Trigger_info.resize(checked_cast<SysInt>(t) + 1);
    return Trigger_info[checked_cast<SysInt>(t)];
  }

  void releaseTriggerInt(DomainInt t, TrigOp op = TO_Default) {
    D_ASSERT(parent != (AbstractConstraint*)BAD_POINTER);
    if(parent != nullptr) {
      Trig_ConRef trig = parent->_parent_map_Trig_ConRef(childpos, checked_cast<SysInt>(t));
      releaseMergedTrigger(trig, op);
    } else {
      releaseMergedTrigger(Trig_ConRef{this, checked_cast<SysInt>(t)}, op);
    }
  }
};

/// Constraint from which other constraints can be inherited. Extends
/// dynamicconstraint to allow children to be dynamic.
class ParentConstraint : public AbstractConstraint {
protected:
  vector<AbstractConstraint*> child_constraints;
  // Maps a dynamic trigger to the constraint which it belongs to.
  // SysInt as they never change, and are always used to index arrays
  vector<SysInt> _dynamicTriggerToConstraint;
  // Offset into array
  vector<SysInt> _dynamicTriggerChildOffset;

  // Maps variables to constraints
  vector<DomainInt> variableToConstraint;
  // Gets start of each constraint
  vector<DomainInt> startOf_constraint;

public:
  SysInt getChildDynamicTrigger(DomainInt p) {
    return _dynamicTriggerToConstraint[checked_cast<SysInt>(p)];
  }

  void passDynTriggerToChild(SysInt trig, DomainDelta dd) {
    SysInt child = getChildDynamicTrigger(trig);
    SysInt offset = _dynamicTriggerChildOffset[child];
    D_ASSERT(trig >= offset);
    child_constraints[child]->propagateDynInt(trig - offset, dd);
  }

  Trig_ConRef _parent_map_Trig_ConRef(SysInt child, SysInt trigger) {
    SysInt offset = _dynamicTriggerChildOffset[child];
    SysInt trignum = offset + trigger;
    if(parent != nullptr) {
      return parent->_parent_map_Trig_ConRef(childpos, trignum);
    } else {
      return Trig_ConRef{this, trignum};
    }
  }

  Con_TrigRef _parent_map_Con_TrigRef(SysInt child, SysInt trigger) {
    SysInt offset = _dynamicTriggerChildOffset[child];
    SysInt trignum = offset + trigger;
    if(parent != nullptr) {
      return parent->_parent_map_Con_TrigRef(childpos, trignum);
    } else {
      return _getTrigRef(trignum);
    }
  }

  ParentConstraint(const vector<AbstractConstraint*> _children) : child_constraints(_children) {
    SysInt varCount = 0;
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i) {
      child_constraints[i]->_setParent(this, i);
      startOf_constraint.push_back(varCount);
      SysInt conSize = child_constraints[i]->getVarsSingleton()->size();
      for(SysInt j = 0; j < conSize; ++j) {
        variableToConstraint.push_back(i);
      }
      varCount += conSize;
    }
  }

  virtual SysInt dynamicTriggerCountWithChildren() {
    SysInt triggerCount = dynamicTriggerCount();
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i)
      triggerCount += child_constraints[i]->dynamicTriggerCountWithChildren();
    return triggerCount;
  }

  virtual void setupDynamicTriggerDatastructures() {
    SysInt currentTriggerCount = dynamicTriggerCount();

    for(SysInt count = 0; count < currentTriggerCount; ++count)
      _dynamicTriggerToConstraint.push_back(child_constraints.size());

    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i) {
      _dynamicTriggerChildOffset.push_back(currentTriggerCount);
      // We need this check to ensure we don't try constructing a "start of
      // trigger" block one off the
      // the end of memory array.
      if(currentTriggerCount == dynamicTriggerCountWithChildren())
        return;

      // Get start child's dynamic triggers.
      child_constraints[i]->setupDynamicTriggerDatastructures();

      SysInt child_trigCount = child_constraints[i]->dynamicTriggerCountWithChildren();

      for(SysInt count = currentTriggerCount; count < currentTriggerCount + child_trigCount;
          ++count)
        _dynamicTriggerToConstraint.push_back(i);

      currentTriggerCount += child_trigCount;
    }
  }

  virtual void setupDynamicTriggers() {
    trig_infoVec.resize(dynamicTriggerCountWithChildren());
  }

  /// Actually creates the dynamic triggers. Calls dynamicTriggerCount from
  /// function to get
  /// the number of triggers required.
  virtual void setup() {
    _setParent(nullptr, -1);
    // Dynamic initialisation
    const SysInt all_trigs = checked_cast<SysInt>(dynamicTriggerCountWithChildren());
    (void)all_trigs;

    D_DATA(DomainInt trigs = dynamicTriggerCount());
    D_ASSERT(trigs >= 0);
    D_ASSERT(all_trigs >= trigs);

    setupDynamicTriggers();
    setupDynamicTriggerDatastructures();
  }

  virtual ~ParentConstraint() {
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i)
      delete child_constraints[i];
  }
};

namespace ConOutput {
inline string print_vars(AbstractConstraint* const& c) {
  return c->fullOutputName();
}
} // namespace ConOutput

#endif
