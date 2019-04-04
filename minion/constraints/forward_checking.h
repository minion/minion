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

#ifndef FORWARD_CHECKING_H_HIUO
#define FORWARD_CHECKING_H_HIUO

#include "../get_info/get_info.h"
#include "../memory_management/reversible_vals.h"
#include "../queue/standard_queue.h"
#include "../triggering/constraint_abstract.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

struct Forward_Checking : public AbstractConstraint {
  ReversibleMonotonicSet FCPruning; // need one bit to support bound vars
  SysInt pruningvar;

  virtual string extendedName() {
    return constraintName() + ":" + child->extendedName();
  }

  virtual string constraintName() {
    return "forwardchecking";
  }

  CONSTRAINT_ARG_LIST1(child);

  AbstractConstraint* child;

  Forward_Checking(AbstractConstraint* _con)
      : FCPruning(1), pruningvar(-1), child(_con), trig1(-1), trig2(-1) {}

  virtual AbstractConstraint* reverseConstraint() {
    return new Forward_Checking(child->reverseConstraint());
  }

  virtual ~Forward_Checking() {
    delete child;
  }

  virtual SysInt dynamicTriggerCount() {
    return 3;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    return child->getSatisfyingAssignment(assignment);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    return child->checkAssignment(v, vSize);
  }

  virtual vector<AnyVarRef> getVars() {
    return child->getVars();
  }

  SysInt trig1, trig2;

  virtual void fullPropagate() {
    SysInt size = child->getVarsSingleton()->size();
    vector<AnyVarRef>* vars = child->getVarsSingleton();

    trig1 = trig2 = -1;

    trig1 = find_newTrigger(-1, -1, 0, size, vars);

    // if all variables assigned
    if(trig1 == -1) {
      if(full_assignmentFailed(size, vars)) {
        getState().setFailed(true);
      }
      return;
    }

    trig2 = find_newTrigger(trig1, -1, 1, size, vars);

    if(trig2 == -1) { // One variable unassigned
      // make sure we put trig2 somewhere!
      trig2 = trig1;

      start_fc_pruning(trig1, size, vars);
    }
    return;
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    SysInt size = child->getVarsSingleton()->size();
    vector<AnyVarRef>* vars = child->getVarsSingleton();

    if(dt == 0) {
      // Find trigger 1 again.

      SysInt temp = find_newTrigger(trig2, trig1, 0, size, vars);

      if(temp != -1) {
        trig1 = temp;
      } else {
        // At most one var is unassigned.
        start_fc_pruning(trig2, size, vars);
      }
      return;
    } else if(dt == 1) {
      // Find trigger 2 again.

      SysInt temp = find_newTrigger(trig1, trig2, 1, size, vars);

      if(temp != -1) {
        trig2 = temp;
      } else {
        // At most one var is unassigned.
        start_fc_pruning(trig1, size, vars);
      }
      return;
    } else if(dt == 2) {
      // If this is a stale trigger, release it.
      if(FCPruning.isMember(0)) {
        releaseTriggerInt(2);
        return;
      } else {
        // Continue doing the bounds pruning.
        fc_pruningBound(pruningvar, size, vars);
        return;
      }
    } else {
      D_ASSERT(false);
    }
  }

  bool full_assignmentFailed(SysInt size, vector<AnyVarRef>* vars) {
    MAKE_STACK_BOX(b, DomainInt, size);
    for(SysInt i = 0; i < size; ++i)
      b.push_back((*vars)[i].assignedValue());

    DomainInt* varptr = 0;
    if(b.size() != 0) {
      varptr = &b[0];
    }

    if(!checkAssignment(varptr, size)) {
      return true; // true means failed.
    } else {
      return false;
    }
  }

  SysInt find_newTrigger(SysInt toavoid, SysInt start, DomainInt dtthis, SysInt size,
                          vector<AnyVarRef>* vars) {
    SysInt i = start + 1;
    for(; i < size; i++) {
      if(i != toavoid && !(*vars)[i].isAssigned()) {
        moveTriggerInt((*vars)[i], dtthis, Assigned);
        return i;
      }
    }

    // Wrap around.
    for(i = 0; i <= start; i++) {
      if(i != toavoid && !(*vars)[i].isAssigned()) {
        moveTriggerInt((*vars)[i], dtthis, Assigned);
        return i;
      }
    }

    return -1;
  }

  void start_fc_pruning(SysInt var, SysInt size, vector<AnyVarRef>* vars) {
    if(!(*vars)[var].isBound()) {
      fc_pruningDiscrete(var, size, vars);
    } else {
      // It's a bound var.
      FCPruning.remove(0); // go into 'pruning' mode
      moveTriggerInt((*vars)[var], 2, DomainChanged);
      pruningvar = var;
      fc_pruningBound(var, size, vars);
    }
  }

  void fc_pruningDiscrete(SysInt var, SysInt size, vector<AnyVarRef>* vars) {
    // Can poke holes so do full FC
    MAKE_STACK_BOX(b, DomainInt, size);
    AnyVarRef v = (*vars)[var];

    for(SysInt i = 0; i < size; ++i) {
      if(i != var) {
        D_ASSERT((*vars)[i].isAssigned());
        b.push_back((*vars)[i].assignedValue());
      } else {
        b.push_back(DomainInt_Skip);
      }
    }

    DomainInt maxval = v.max();

    for(DomainInt value = v.min(); value <= maxval; value++) {
      if(v.inDomain(value)) {
        b[var] = value;
        if(!checkAssignment(&b[0], size)) {
          v.removeFromDomain(value);
        }
      }
    }
  }

  void fc_pruningBound(SysInt var, SysInt size, vector<AnyVarRef>* vars) {

    MAKE_STACK_BOX(b, DomainInt, size);
    AnyVarRef v = (*vars)[var];

    for(SysInt i = 0; i < size; ++i) {
      if(i != var) {
        D_ASSERT((*vars)[i].isAssigned());
        b.push_back((*vars)[i].assignedValue());
      } else {
        b.push_back(DomainInt_Skip);
      }
    }

    DomainInt maxval = v.max();

    // Scan up from lower bound.
    for(DomainInt value = v.min(); value <= maxval; value++) {
      b[var] = value;
      if(!checkAssignment(&b[0], size)) {
        v.setMin(value + 1);
      } else {
        break;
      }
    }

    DomainInt minval = v.min();

    // Scan down from upper bound
    for(DomainInt value = v.max(); value >= minval; value--) {
      b[var] = value;
      if(!checkAssignment(&b[0], size)) {
        v.setMax(value - 1);
      } else {
        break;
      }
    }
  }
};

inline AbstractConstraint* forwardCheckingCon(AbstractConstraint* c) {
  return new Forward_Checking(c);
}

inline AbstractConstraint* BuildCT_FORWARD_CHECKING(ConstraintBlob& bl) {
  D_ASSERT(bl.internal_constraints.size() == 1);
  return forwardCheckingCon(build_constraint(bl.internal_constraints[0]));
}

/* JSON
{ "type": "constraint",
  "name": "forwardchecking",
  "internal_name": "CT_FORWARD_CHECKING",
  "args": [ "read_constraint" ]
}
*/

#endif
