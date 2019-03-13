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

/** @help constraints;abs Description
The constraint

   abs(x,y)

makes sure that x=|y|, i.e. x is the absolute value of y.
*/

/** @help constraints;abs Reference
help constraints abs
*/

#ifndef CONSTRAINT_ABS_H
#define CONSTRAINT_ABS_H

#include "constraint_checkassign.h"

// X = abs(Y)
template <typename AbsVarRef1, typename AbsVarRef2>
struct AbsConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "abs";
  }

  CONSTRAINT_ARG_LIST2(var1, var2);

  AbsVarRef1 var1;
  AbsVarRef2 var2;
  AbsConstraint(AbsVarRef1 _var1, AbsVarRef2 _var2) : var1(_var1), var2(_var2) {}

  virtual SysInt dynamicTriggerCount() {
    return 4;
  }

  void trigger_setup() {
    moveTriggerInt(var1, 0, UpperBound);
    moveTriggerInt(var1, 1, LowerBound);
    moveTriggerInt(var2, 2, UpperBound);
    moveTriggerInt(var2, 3, LowerBound);
  }

  virtual void fullPropagate() {
    trigger_setup();
    var1.setMin(0);
    for(SysInt i = 0; i < 4 && !getState().isFailed(); ++i)
      propagateDynInt(i, DomainDelta::empty());
  }

  // Assume values passed in in order.
  DomainInt absmax(DomainInt x, DomainInt y) {
    D_ASSERT(x <= y);
    if(x < 0)
      x *= -1;
    if(y < 0)
      y *= -1;
    return max(x, y);
  }

  // Assume values passed in in order.
  DomainInt absmin(DomainInt x, DomainInt y) {
    D_ASSERT(x <= y);
    if(x <= 0) {
      if(y >= 0)
        return 0;
      else {
        return -y;
      }
    } else
      return x;
  }

  virtual void propagateDynInt(SysInt i, DomainDelta) {
    // Assume this in the algorithm.
    D_ASSERT(var1.min() >= 0);

    PROP_INFO_ADDONE(Abs);
    switch(i) {
    case 0: // var1 upper
      var2.setMax(var1.max());
      var2.setMin(-var1.max());
      return;
    case 1: // var1 lower
      if(var2.max() < var1.min())
        var2.setMax(-var1.min());
      if(var2.min() > -var1.min())
        var2.setMin(var1.min());
      else
        var2.setMin(-var1.max());
      return;
    case 2: // var 2 upper
    case 3: // var 2 lower
      var1.setMax(absmax(var2.min(), var2.max()));
      var1.setMin(absmin(var2.min(), var2.max()));
      if(var2.min() > -var1.min())
        var2.setMin(var1.min());
      return;
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 2);
    if(v[1] >= 0)
      return v[0] == v[1];
    else
      return v[0] == -v[1];
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    DomainInt xDomMax = var1.max();
    DomainInt yDomMax = max(abs(var2.min()), abs(var2.max()));
    DomainInt domMax = min(xDomMax, yDomMax);
    DomainInt domMin = max(DomainInt(0), max(var1.min(), var2.min()));

    for(DomainInt i = domMin; i <= domMax; ++i) {
      if(var1.inDomain(i)) {
        if(var2.inDomain(i)) {
          assignment.push_back(make_pair(0, i));
          assignment.push_back(make_pair(1, i));
          return true;
        } else if(var2.inDomain(-i)) {
          assignment.push_back(make_pair(0, i));
          assignment.push_back(make_pair(1, -i));
          return true;
        }
      }
    }
    return false;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(2);
    vars.push_back(var1);
    vars.push_back(var2);
    return vars;
  }

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverseConstraint() {
    return forwardCheckNegation(this);
  }
};

template <typename EqualVarRef1, typename EqualVarRef2>
AbstractConstraint* AbsCon(EqualVarRef1 var1, EqualVarRef2 var2) {
  return new AbsConstraint<EqualVarRef1, EqualVarRef2>(var1, var2);
}
#endif

template <typename T1, typename T2>
AbstractConstraint* BuildCT_ABS(const T1& t1, const T2& t2, ConstraintBlob&) {
  return AbsCon(t1[0], t2[0]);
}

/* JSON
{ "type": "constraint",
  "name": "abs",
  "internal_name": "CT_ABS",
  "args": [ "read_var", "read_var" ]
}
*/
