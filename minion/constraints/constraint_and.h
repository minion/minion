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

#ifndef CONSTRAINT_AND_H
#define CONSTRAINT_AND_H

#include "constraint_checkassign.h"

/// var1 /\ var2 = var3
template <typename VarRef1, typename VarRef2, typename VarRef3>
struct AndConstraint : public AbstractConstraint {

  virtual string extendedName() {
    return "product: and";
  }

  virtual string constraintName() {
    return "product";
  }

  CONSTRAINT_ARG_LIST3(var1, var2, var3);

  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;
  AndConstraint(VarRef1 _var1, VarRef2 _var2, VarRef3 _var3)
      : var1(_var1), var2(_var2), var3(_var3) {
    CHECK(var1.initialMin() == 0, "The 'and' constraint works only Booleans");
    CHECK(var1.initialMax() == 1, "The 'and' constraint works only Booleans");
    CHECK(var2.initialMin() == 0, "The 'and' constraint works only Booleans");
    CHECK(var2.initialMax() == 1, "The 'and' constraint works only Booleans");
    CHECK(var3.initialMin() == 0, "The 'and' constraint works only Booleans");
    CHECK(var3.initialMax() == 1, "The 'and' constraint works only Booleans");
  }

  virtual SysInt dynamicTriggerCount() {
    return 6;
  }

  void setup_triggers() {
    moveTriggerInt(var1, 0, LowerBound);
    moveTriggerInt(var2, 1, LowerBound);
    moveTriggerInt(var3, 2, LowerBound);
    moveTriggerInt(var1, 3, UpperBound);
    moveTriggerInt(var2, 4, UpperBound);
    moveTriggerInt(var3, 5, UpperBound);
  }

  virtual void propagateDynInt(SysInt i, DomainDelta) {
    PROP_INFO_ADDONE(And);
    switch(checked_cast<SysInt>(i)) {
    case 0:
      if(var2.isAssignedValue(true))
        var3.assign(true);
      else {
        if(var3.isAssignedValue(false))
          var2.assign(false);
      }
      break;

    case 1:
      if(var1.isAssignedValue(true))
        var3.assign(true);
      else {
        if(var3.isAssignedValue(false))
          var1.assign(false);
      }
      break;

    case 2:
      var1.assign(true);
      var2.assign(true);
      break;

    case 3:
    case 4: var3.assign(false); break;

    case 5:
      if(var1.isAssignedValue(true))
        var2.assign(false);
      else {
        if(var2.isAssignedValue(true))
          var1.assign(false);
      }
      break;
    }
  }

  virtual void fullPropagate() {
    setup_triggers();
    if(var1.isAssignedValue(false) || var2.isAssignedValue(false))
      var3.assign(false);

    if(var1.isAssignedValue(true) && var2.isAssignedValue(true))
      var3.assign(true);

    if(var3.isAssignedValue(false)) {
      if(var1.isAssignedValue(true))
        var2.assign(false);
      if(var2.isAssignedValue(true))
        var1.assign(false);
    }

    if(var3.isAssignedValue(true)) {
      var1.assign(true);
      var2.assign(true);
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt v_size) {
    D_ASSERT(v_size == 3);
    return ((v[0] != 0) && (v[1] != 0)) == (v[2] != 0);
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    if(var3.max() == 1) {
      if(var1.max() == 1 && var2.max() == 1) {
        assignment.push_back(make_pair(0, 1));
        assignment.push_back(make_pair(1, 1));
        assignment.push_back(make_pair(2, 1));
        return true;
      }
    }

    if(var3.min() == 0) {
      if(var2.min() == 0) {
        assignment.push_back(make_pair(1, 0));
        assignment.push_back(make_pair(2, 0));
        return true;
      }

      if(var1.min() == 0) {
        assignment.push_back(make_pair(0, 0));
        assignment.push_back(make_pair(2, 0));
        return true;
      }
    }
    return false;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> v;
    v.push_back(var1);
    v.push_back(var2);
    v.push_back(var3);
    return v;
  }

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverseConstraint() {
    return forward_check_negation(this);
  }
};

template <typename VarRef1, typename VarRef2, typename VarRef3>
AbstractConstraint* AndCon(VarRef1 var1, VarRef2 var2, VarRef3 var3) {
  return (new AndConstraint<VarRef1, VarRef2, VarRef3>(var1, var2, var3));
}

#endif
