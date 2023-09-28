// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef REIFY_TRUE_H
#define REIFY_TRUE_H

#include "../get_info/get_info.h"
#include "../memory_management/reversible_vals.h"
#include "../queue/standard_queue.h"
#include "../triggering/constraint_abstract.h"
#include "dynamic_new_and.h"
#include "unary/dynamic_literal.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

template <typename BoolVar, bool DoWatchAssignment>
struct reify_true : public ParentConstraint {
  virtual string extendedName() {
    return constraintName() + ":" + child_constraints[0]->extendedName();
  }

  virtual string constraintName() {
    if(DoWatchAssignment)
      return "reifyimply";
    else
      return "reifyimply-quick";
  }

  CONSTRAINT_ARG_LIST2(child_constraints[0], rar_var);

  BoolVar rar_var;
  bool constraintLocked;

  Reversible<bool> fullPropagate_called;

  reify_true(AbstractConstraint* _poscon, BoolVar _rar_var)
      : ParentConstraint({_poscon}),
        rar_var(_rar_var),
        constraintLocked(false),
        fullPropagate_called(false) {
    CHECK(rar_var.initialMin() >= 0 && rar_var.initialMax() <= 1,
          "reifyimply only works on Boolean variables");
  }

  // (var -> C) is equiv to (!var \/ C), so reverse is (var /\ !C)
  virtual AbstractConstraint* reverseConstraint() {
    vector<AbstractConstraint*> con;
    con.push_back(new WatchLiteralConstraint<BoolVar>(rar_var, 1));
    con.push_back(child_constraints[0]->reverseConstraint());
    return new Dynamic_AND(con);
  }

  virtual SysInt dynamicTriggerCount() {
    if(DoWatchAssignment)
      return child_constraints[0]->getVarsSingleton()->size() * 2 + 1;
    else
      return 1;
  }

  SysInt reify_varTrigger() {
    if(DoWatchAssignment)
      return child_constraints[0]->getVarsSingleton()->size() * 2;
    else
      return 0;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    if(rar_var.inDomain(0)) {
      D_ASSERT(getVars()[child_constraints[0]->getVarsSingleton()->size()].inDomain(0));
      assignment.push_back(make_pair(child_constraints[0]->getVarsSingleton()->size(), 0));
      return true;
    } else {
      D_ASSERT(rar_var.inDomain(1));
      bool ret = child_constraints[0]->getSatisfyingAssignment(assignment);
      if(ret) {
        assignment.push_back(make_pair(child_constraints[0]->getVarsSingleton()->size(), 1));
        return true;
      } else
        return false;
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    DomainInt backVal = *(v + checked_cast<SysInt>(vSize - 1));
    // v.pop_back();
    if(backVal == 1)
      return child_constraints[0]->checkAssignment(v, vSize - 1);
    else
      return (backVal == 0);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vec = child_constraints[0]->getVars();
    vec.push_back(rar_var);
    return vec;
  }

  virtual void specialCheck() {
    D_ASSERT(constraintLocked);
    P("Special Check!");
    constraintLocked = false;
    child_constraints[0]->fullPropagate();
    fullPropagate_called = true;
  }

  virtual void specialUnlock() {
    D_ASSERT(constraintLocked);
    P("Special unlock!");
    constraintLocked = false;
  }

  void reify_var_pruned() {
    if(!rar_var.isAssigned() || rar_var.assignedValue() == 0)
      return;
    D_ASSERT(rar_var.assignedValue() == 1);
    P("rarvar assigned to 1- Do full propagate");
    constraintLocked = true;
    getQueue().pushSpecialTrigger(this);
  }

  virtual void propagateDynInt(SysInt trig, DomainDelta dd) {
    PROP_INFO_ADDONE(ReifyTrue);

    P("Dynamic prop start");
    if(constraintLocked)
      return;

    const SysInt dt = 0;

    if(trig == reify_varTrigger()) {
      reify_var_pruned();
      return;
    }

    if(DoWatchAssignment && trig >= dt &&
       trig < dt + dynamicTriggerCount()) { // Lost assignment, but don't
                                              // replace when rar_var=0
      P("Triggered on an assignment watch");
      if(trig == dt + dynamicTriggerCount() - 1)
        abort();

      if(!fullPropagate_called && !rar_var.isAssigned()) {
        bool flag;
        GET_ASSIGNMENT(assignment, child_constraints[0]);
        PROP_INFO_ADDONE(ReifyImplyGetSatAssg);

        P("Find new assignment");
        if(!flag) { // No satisfying assignment to constraint
          P("Failed!");
          rar_var.assign(0);
          return;
        }
        P("Found new assignment");
        watch_assignment(assignment, *(child_constraints[0]->getVarsSingleton()), dt);
      }
      return;
    }

    if(fullPropagate_called) {
      P("Pass triggers to children");
      D_ASSERT(rar_var.isAssigned() && rar_var.assignedValue() == 1);
      passDynTriggerToChild(trig, dd);
      // child_constraints[0]->propagateDynInt(trig);
    } else {
      P("Remove unused trigger");
      // This is an optimisation.
      releaseTriggerInt(trig);
    }
  }

  template <typename T, typename Vars, typename Trigger>
  void watch_assignment(const T& assignment, Vars& vars, Trigger trig) {
    for(SysInt i = 0; i < (SysInt)assignment.size(); ++i) {
      D_ASSERT(vars[assignment[i].first].inDomain(assignment[i].second));
      if(vars[assignment[i].first].isBound()) {
        moveTriggerInt(vars[assignment[i].first], trig + i, DomainChanged);
      } else {
        moveTriggerInt(vars[assignment[i].first], trig + i, DomainRemoval, assignment[i].second);
      }
    }
  }

  virtual void fullPropagate() {
    P("Full prop");
    D_ASSERT(!getState().isFailed());
    P(child_constraints[0]->constraintName());
    D_ASSERT(rar_var.min() >= 0);
    D_ASSERT(rar_var.max() <= 1);

    moveTriggerInt(rar_var, reify_varTrigger(), LowerBound);

    if(rar_var.isAssigned() && rar_var.assignedValue() == 1) {
      child_constraints[0]->fullPropagate();
      fullPropagate_called = true;
      return;
    }

    const SysInt dt = 0;
    SysInt dtCount = dynamicTriggerCount();
    // Clean up triggers (skip the one watching the reification variable)
    for(SysInt i = 0; i < dtCount - 1; ++i)
      releaseTriggerInt(i);

    if(DoWatchAssignment && !rar_var.isAssigned()) // don't place when rar_var=0
    {
      bool flag;
      GET_ASSIGNMENT(assignment, child_constraints[0]);
      PROP_INFO_ADDONE(ReifyImplyGetSatAssg);
      if(!flag) { // No satisfying assignment to constraint
        P("Assigning reifyvar to 0");
        rar_var.assign(0);
        return;
      }
      watch_assignment(assignment, *(child_constraints[0]->getVarsSingleton()), dt);
    }
  }
};

template <typename BoolVar>
AbstractConstraint* truereifyCon(AbstractConstraint* c, BoolVar var) {
  return new reify_true<BoolVar, true>(&*c, var);
}

template <typename BoolVar>
AbstractConstraint* truereifyQuickCon(AbstractConstraint* c, BoolVar var) {
  return new reify_true<BoolVar, false>(&*c, var);
}

template <typename VarArray>
inline AbstractConstraint* BuildCT_REIFYIMPLY(const VarArray& vars, ConstraintBlob& bl) {
  D_ASSERT(bl.internal_constraints.size() == 1);
  D_ASSERT(vars.size() == 1);
  return truereifyCon(build_constraint(bl.internal_constraints[0]), vars[0]);
}

/* JSON
{ "type": "constraint",
  "name": "reifyimply",
  "internal_name": "CT_REIFYIMPLY",
  "args": [ "read_constraint", "read_var" ]
}
*/

template <typename VarArray>
inline AbstractConstraint* BuildCT_REIFYIMPLY_QUICK(const VarArray& vars, ConstraintBlob& bl) {
  D_ASSERT(bl.internal_constraints.size() == 1);
  D_ASSERT(vars.size() == 1);
  return truereifyQuickCon(build_constraint(bl.internal_constraints[0]), vars[0]);
}

/* JSON
{ "type": "constraint",
  "name": "reifyimply-quick",
  "internal_name": "CT_REIFYIMPLY_QUICK",
  "args": [ "read_constraint", "read_var" ]
}
*/

#endif
