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

/** @help constraints;reify References
See
   help constraints reification
*/

/** @help constraints;reifyimply References
See
   help constraints reification
*/

/** @help constraints;reification Description
Reification is provided in two forms: reify and reifyimply.

   reify(constraint, r) where r is a 0/1 var

ensures that r is set to 1 if and only if constraint is satisfied. That is, if r
is 0 the constraint must NOT be satisfied; and if r is 1 it must be satisfied as
normal. Conversely, if the constraint is satisfied then r must be 1, and if not
then r must be 0.

   reifyimply(constraint, r)

only checks that if r is set to 1 then constraint must be satisfied. If r is not
1, constraint may be either satisfied or unsatisfied. Furthermore r is never set
by propagation, only by search; that is, satisfaction of constraint does not
affect the value of r.
*/

/** @help constraints;reification Notes
ALMOST ALL constraints are are reifiable. Individual constraint entries mention
if the constraint is NOT reifiable.

ALL constraints are reifyimplyable.
*/

// Note: The whole constraintLocked thing is for the following case:
// Consider the following events are on the queue:
// "rareify boolean is assigned, Y is assigned"
// Now "rareify boolean is assigned" causes fullPropagate to be called for
// the constraint. It will set up it's data structures based on the current
// assignment. Then later it will be given Y is assigned, but have already
// possibly used that. Confusion follows. Therefore when we want to propagate
// the function, we "lock" it until the queue empties, then start ping
// the constraint.

#ifndef REIFY_H
#define REIFY_H

#include "../get_info/get_info.h"
#include "../memory_management/reversible_vals.h"
#include "../queue/standard_queue.h"
#include "../triggering/constraint_abstract.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

#define NEWREIFY
//#define NODETRICK   // This is broken! leave switched off.

#ifdef NEWREIFY

// In here variables are numbered from child_constraints[0].getVars(), then
// child_constraints[1].getVars(), then reify_var

template <typename BoolVar>
struct reify : public ParentConstraint {

  virtual string extendedName() {
    return constraintName() + ":" + child_constraints[0]->extendedName();
  }

  virtual string constraintName() {
    return "reify";
  }

  CONSTRAINT_ARG_LIST2(child_constraints[0], reify_var);

  BoolVar reify_var;
  SysInt reify_varNum;

  bool constraintLocked;
  Reversible<bool> fullPropagate_called;

#ifdef NODETRICK
  unsigned long long reifysetnode;
#endif

  SysInt dtcount;
  SysInt c0vars; // how many vars for child_constraints[0]

  typedef vector<vector<pair<DomainInt, DomainInt>>> triggerpairstype;
  D_DATA(triggerpairstype triggerpairs);

  reify(AbstractConstraint* _poscon, BoolVar _rar_var)
      : ParentConstraint({_poscon, _poscon->reverseConstraint()}),
        reify_var(_rar_var),
        constraintLocked(false),
        fullPropagate_called(false) {
    CHECK(reify_var.initialMin() >= 0 && reify_var.initialMax() <= 1,
          "reify only works on Boolean variables");
#ifdef NODETRICK
    numeric_limits<unsigned long long> ull;
    reifysetnode = ull.max();
#endif
    // assume for the time being that the two child constraints have the same
    // number of vars.
    reify_varNum = child_constraints[0]->getVarsSingleton()->size() +
                    child_constraints[1]->getVarsSingleton()->size();
    // dtcount=dynamicTriggerCount();
    dtcount = child_constraints[0]->getVarsSingleton()->size() * 2 +
              child_constraints[1]->getVarsSingleton()->size() * 2;
    c0vars = child_constraints[0]->getVarsSingleton()->size();

    D_DATA(triggerpairs.resize(2));
  }

  virtual AbstractConstraint* reverseConstraint() {
    // reverse it by swapping the positive and negative constraints.
    // we call 'reverseConstraint' here to force a new copy of the constraint
    return new reify<BoolVar>(child_constraints[0]->reverseConstraint(), reify_var);
  }

  virtual SysInt dynamicTriggerCount() {
    return dtcount + 1; // *2 for each child constraint.
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    if(reify_var.inDomain(1)) {
      bool flag = child_constraints[0]->getSatisfyingAssignment(assignment);
      if(flag) {
        assignment.push_back(make_pair(reify_varNum, 1));
        return true;
      }
    }
    assignment.clear();
    if(reify_var.inDomain(0)) {
      bool flag = child_constraints[1]->getSatisfyingAssignment(assignment);
      if(flag) {
        for(SysInt i = 0; i < (SysInt)assignment.size(); ++i)
          assignment[i].first += c0vars;
        assignment.push_back(make_pair(reify_varNum, 0));
        return true;
      }
    }
    return false;
  }

  virtual BOOL checkAssignment(DomainInt* vals, SysInt vSize) {
    DomainInt backVal = *(vals + vSize - 1);
    if(backVal == 1) {
      return child_constraints[0]->checkAssignment(vals, c0vars);
    } else if(backVal == 0) {
      vals += c0vars;
      return child_constraints[1]->checkAssignment(vals, (dtcount / 2) - c0vars);
    } else
      return false;
  }

  virtual vector<AnyVarRef> getVars() {
    // Push both sets of vars, then reify var.
    vector<AnyVarRef> vec0 = *child_constraints[0]->getVarsSingleton();
    vector<AnyVarRef> vec1 = *child_constraints[1]->getVarsSingleton();
    vector<AnyVarRef> c;
    c.reserve(vec0.size() + vec1.size() + 1);
    for(SysInt i = 0; i < (SysInt)vec0.size(); i++)
      c.push_back(vec0[i]);
    for(SysInt i = 0; i < (SysInt)vec1.size(); i++)
      c.push_back(vec1[i]);
    c.push_back(reify_var);
    return c;
  }

  virtual void specialCheck() {
    D_ASSERT(constraintLocked);
    P("Special Check!");
    constraintLocked = false;
    D_ASSERT(reify_var.isAssigned() &&
             (reify_var.assignedValue() == 0 || reify_var.assignedValue() == 1));
    if(reify_var.inDomain(0)) {
      child_constraints[1]->fullPropagate();
    } else {
      child_constraints[0]->fullPropagate();
    }
    fullPropagate_called = true;
  }

  virtual void specialUnlock() {
    D_ASSERT(constraintLocked);
    P("Special unlock!");
    constraintLocked = false;
  }

  void reify_varAssigned() {
    if(!fullPropagate_called) {
      P("reifyvar assigned - Do full propagate");
#ifdef NODETRICK
      if(reifysetnode == getState().getNodeCount()) {
        numeric_limits<unsigned long long> ull; // I hope the compiler will get rid fo this..
        reifysetnode = ull.max();               // avoid this happening more than once.
        return;
      }
#endif

      constraintLocked = true;
      getQueue().pushSpecialTrigger(this);
    }
  }

  virtual void propagateDynInt(SysInt trig, DomainDelta dd) {
    PROP_INFO_ADDONE(Reify);
    P("Dynamic prop start");
    if(constraintLocked)
      return;

    const SysInt _dt = 0;
    // SysInt numtriggers=dynamicTriggerCount();

    if(!fullPropagate_called) {
      if(trig >= _dt && trig < (_dt + (c0vars * 2))) { // Lost assignments for positive constraint.
        P("Triggered on an assignment watch for the positive child constraint");

        bool flag;
        PROP_INFO_ADDONE(ReifyPropGetAssgPosCon);
        GET_ASSIGNMENT(assignment, child_constraints[0]);

        P("Find new assignment");
        if(!flag) { // No satisfying assignment to constraint
          P("Failed!");
          reify_var.assign(0);

#ifdef NODETRICK
          reifysetnode = getState().getNodeCount();
#endif

          return;
        }
        P("Found new assignment");
        watch_assignment(assignment, *(child_constraints[0]->getVarsSingleton()), 0, c0vars * 2);

        return;
      } else if(trig >= (_dt + (c0vars * 2)) &&
                trig < _dt + dtcount) { // Lost assignments for negative constraint.
        P("Triggered on an assignment watch for the negative child constraint");

        bool flag;
        GET_ASSIGNMENT(assignment, child_constraints[1]);
        PROP_INFO_ADDONE(ReifyPropGetAssgNegCon);

        P("Find new assignment");
        if(!flag) { // No satisfying assignment to constraint
          P("Failed!");
          reify_var.assign(1);

#ifdef NODETRICK
          reifysetnode = getState().getNodeCount();
#endif

          return;
        }
        P("Found new assignment");
        watch_assignment(assignment, *(child_constraints[1]->getVarsSingleton()), c0vars * 2,
                         dtcount);
        return;
      } else if(trig == _dt + dtcount) {
        reify_varAssigned();
        return;
      } else {
        P("Remove unused trigger");
        // This is an optimisation. Remove a trigger from stage 2.
        releaseTriggerInt(trig);
      }
    } else // fullPropagate_called
    {
      if(trig >= _dt && trig < _dt + dtcount) { // is it a trigger from stage 1 .. if so, ignore.
        P("In stage 2, ignoring trigger from stage 1");
        return;
      }
      P("Pass triggers to children");
      D_ASSERT(reify_var.isAssigned());

      SysInt child = getChildDynamicTrigger(trig);
      if(reify_var.assignedValue() == child) {
        P("Removing leftover trigger from other child constraint");
        releaseTriggerInt(trig);
        return;
      }
      passDynTriggerToChild(trig, dd);
      // child_constraints[getChildDynamicTrigger(trig)]->propagateDynInt(trig);
    }
  }

  template <typename T, typename Vars>
  void watch_assignment(const T& assignment, Vars& vars, DomainInt trig, DomainInt endtrig) {
    for(SysInt i = 0; i < (SysInt)assignment.size(); ++i) {
      const SysInt aif = checked_cast<SysInt>(assignment[i].first);
      D_ASSERT(vars[aif].inDomain(assignment[i].second));
      D_ASSERT(trig + i < endtrig);
      if(vars[aif].isBound()) {
        moveTriggerInt(vars[aif], trig + i, DomainChanged);
      } else {
        moveTriggerInt(vars[aif], trig + i, DomainRemoval, assignment[i].second);
      }
    }
    // clear a contiguous block of used triggers up to (not including) endtrig
    for(SysInt i = assignment.size(); (trig + i) < endtrig; i++) {
      /// XXX : This is inefficent, but required for constant variables
      /*  if(!(trig+i)->isAttached())
        {
            D_DATA(firstunattached=i);
            break;
        } */
      releaseTriggerInt(trig + i);
    }
  }

  virtual void fullPropagate() {
    P("Full prop");
    P("reify " << child_constraints[0]->constraintName());
    P("negation: " << child_constraints[1]->constraintName());

    D_ASSERT(reify_var.min() >= 0);
    D_ASSERT(reify_var.max() <= 1);
    if(getState().isFailed())
      return;

    moveTriggerInt(reify_var, dtcount, Assigned);

    if(reify_var.isAssigned()) {
      if(reify_var.assignedValue() == 1) {
        child_constraints[0]->fullPropagate();
      } else {
        child_constraints[1]->fullPropagate();
      }
      fullPropagate_called = true;
      return;
    }

    // Clean up triggers
    for(SysInt i = 0; i < dtcount; ++i)
      releaseTriggerInt(i);

    bool flag;
    GET_ASSIGNMENT(assignment0, child_constraints[0]);
    PROP_INFO_ADDONE(ReifyFullPropGetAssgPosCon);
    if(!flag) { // No satisfying assignment to constraint
      reify_var.assign(0);

#ifdef NODETRICK
      reifysetnode = getState().getNodeCount();
#endif

      return;
    }
    PROP_INFO_ADDONE(ReifyFullPropGetAssgNegCon);
    GET_ASSIGNMENT(assignment1, child_constraints[1]);
    if(!flag) { // No satisfying assignment to constraint
      reify_var.assign(1);
#ifdef NODETRICK
      reifysetnode = getState().getNodeCount();
#endif

      return;
    }

    watch_assignment(assignment0, *(child_constraints[0]->getVarsSingleton()), 0, (c0vars * 2));
    watch_assignment(assignment1, *(child_constraints[1]->getVarsSingleton()), (c0vars * 2),
                     dtcount);
  }
};

#else

#error OLD_REIFY is gone

#endif
// end of ifdef NEWREIFY

template <typename BoolVar>
reify<BoolVar>* reifyCon(AbstractConstraint* c, BoolVar var) {
  return new reify<BoolVar>(&*c, var);
}

template <typename VarArray>
inline AbstractConstraint* BuildCT_REIFY(const VarArray& vars, ConstraintBlob& bl) {
  ConstraintType type = bl.internal_constraints[0].constraint->type;
  switch(type) {
  case CT_GACEQ:
  case CT_EQ:
    // Code just for GACEQ case
    if(type == CT_GACEQ) {
      ConstraintBlob blob(bl.internal_constraints[0]);
      auto bound1 = getInitialBoundsFromVar(blob.vars[0][0]);
      auto bound2 = getInitialBoundsFromVar(blob.vars[1][0]);
      auto minbound = std::max(bound1.first, bound2.first);
      auto maxbound = std::min(bound1.second, bound2.second);
      if(minbound < 0 || maxbound > 1)
        break;
    }
    // Common code for GACEQ and EQ
    {
      ConstraintBlob blob(bl.internal_constraints[0]);
      blob.vars.push_back(makeVec(bl.vars[0][0]));
      blob.constraint = get_constraint(CT_EQ_REIFY);
      return build_constraint(blob);
    }
  case CT_DISEQ: {
    ConstraintBlob blob(bl.internal_constraints[0]);
    blob.vars.push_back(makeVec(bl.vars[0][0]));
    blob.constraint = get_constraint(CT_DISEQ_REIFY);
    return build_constraint(blob);
  }
  case CT_MINUSEQ: {
    ConstraintBlob blob(bl.internal_constraints[0]);
    blob.vars.push_back(makeVec(bl.vars[0][0]));
    blob.constraint = get_constraint(CT_MINUSEQ_REIFY);
    return build_constraint(blob);
  }
  default:; // to hide warnings
  }

  return reifyCon(build_constraint(bl.internal_constraints[0]), vars[0]);
}

/* JSON
{ "type": "constraint",
  "name": "reify",
  "internal_name": "CT_REIFY",
  "args": [ "read_constraint", "read_var" ]
}
*/

#endif
