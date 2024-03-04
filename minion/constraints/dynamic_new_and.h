// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0







#ifndef DYNAMIC_WATCHED_AND_NEW_H
#define DYNAMIC_WATCHED_AND_NEW_H

#include "../get_info/get_info.h"
#include "../memory_management/reversible_vals.h"
#include "../queue/standard_queue.h"
#include "../triggering/constraint_abstract.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

// Similar to watched or, but has no watching phase, just propagates all
// the time, and propagates all constraints of course.

struct Dynamic_AND : public ParentConstraint {
  virtual string constraintName() {
    return "watched-and";
  }

  CONSTRAINT_ARG_LIST1(child_constraints);

  bool constraintLocked;
  SysInt propagatedTo;
  Dynamic_AND(vector<AbstractConstraint*> _con)
      : ParentConstraint(_con), constraintLocked(false) {}

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i) {
      if(!child_constraints[i]->checkAssignment(
             v + checked_cast<SysInt>(startOf_constraint[i]),
             child_constraints[i]->getVarsSingleton()->size()))
        return false;
    }
    return true;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    // get all satisfying assignments of child constraints and stick
    // them together. Even if they contradict each other.
    typedef pair<SysInt, DomainInt> temptype;
    MAKE_STACK_BOX(localassignment, temptype, assignment.capacity());
    P("GetSat for And");
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i) {
      localassignment.clear();
      bool flag = child_constraints[i]->getSatisfyingAssignment(localassignment);
      if(!flag) {
        assignment.clear();
        return false;
      }
      P(localassignment[0] << ":" << localassignment[1]);
      for(SysInt j = 0; j < (SysInt)localassignment.size(); j++) {
        assignment.push_back(
            make_pair(checked_cast<SysInt>(localassignment[j].first + startOf_constraint[i]),
                      localassignment[j].second));
        D_ASSERT((*(this->getVarsSingleton()))[checked_cast<SysInt>(localassignment[j].first +
                                                                      startOf_constraint[i])]
                     .inDomain(localassignment[j].second));
        D_ASSERT((*(child_constraints[i]
                        ->getVarsSingleton()))[checked_cast<SysInt>(localassignment[j].first)]
                     .inDomain(localassignment[j].second));
      }
    }
    return true;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vecs;
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i) {
      vector<AnyVarRef>* varPtr = child_constraints[i]->getVarsSingleton();
      vecs.insert(vecs.end(), varPtr->begin(), varPtr->end());
    }
    return vecs;
  }

  virtual SysInt dynamicTriggerCount() {
    return 0;
  }

  virtual void specialCheck() {
    D_ASSERT(constraintLocked);
    D_ASSERT(!getState().isFailed());
    P("Full propagating all constraints in AND");
    if(child_constraints.size() == 0) {
      constraintLocked = false;
      return;
    }

    child_constraints[propagatedTo]->fullPropagate();
    propagatedTo++;
    if(propagatedTo != (SysInt)child_constraints.size())
      getQueue().pushSpecialTrigger(this);
    else {
      constraintLocked = false;
    }
  }

  virtual void specialUnlock() {
    D_ASSERT(constraintLocked);
    constraintLocked = false;
  }

  virtual void propagateDynInt(SysInt trig, DomainDelta dd) {
    // PROP_INFO_ADDONE(WatchedOr);
    P("Prop");
    P("Locked:" << constraintLocked);
    // pass the trigger down
    P("Propagating child");
    // need to know which child to prop.
    SysInt child = getChildDynamicTrigger(trig);
    if(!constraintLocked || child < propagatedTo) {
      passDynTriggerToChild(trig, dd);
      // child_constraints[child]->propagateDynInt(trig);
    }
  }

  virtual void fullPropagate() {
    P("AND Full Propagate");
    // push it on the special queue to be fullPropagated later.
    D_ASSERT(!constraintLocked);
    constraintLocked = true;
    propagatedTo = 0;
    getQueue().pushSpecialTrigger(this);
  }

  virtual AbstractConstraint* reverseConstraint();
};

#include "dynamic_new_or.h"

inline AbstractConstraint* Dynamic_AND::reverseConstraint() { // OR of the reverse of all the child
                                                               // constraints..
  vector<AbstractConstraint*> con;
  for(SysInt i = 0; i < (SysInt)child_constraints.size(); i++) {
    con.push_back(child_constraints[i]->reverseConstraint());
  }
  return new Dynamic_OR(con);
}

inline AbstractConstraint* BuildCT_WATCHED_NEW_AND(ConstraintBlob& bl) {
  vector<AbstractConstraint*> cons;
  for(SysInt i = 0; i < (SysInt)bl.internal_constraints.size(); ++i)
    cons.push_back(build_constraint(bl.internal_constraints[i]));

  return new Dynamic_AND(cons);
}

/* JSON
{ "type": "constraint",
  "name": "watched-and",
  "internal_name": "CT_WATCHED_NEW_AND",
  "args": [ "read_constraint_list" ]
}
*/
#endif
