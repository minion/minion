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



















#ifndef CONSTRAINT_GACEQUAL_H
#define CONSTRAINT_GACEQUAL_H

#include "constraint_equal.h"

template <typename EqualVarRef1, typename EqualVarRef2>
struct GACEqualConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "gaceq";
  }

  EqualVarRef1 var1;
  EqualVarRef2 var2;

  CONSTRAINT_ARG_LIST2(var1, var2);

  SysInt dvar2;

  GACEqualConstraint(EqualVarRef1 _var1, EqualVarRef2 _var2) : var1(_var1), var2(_var2) {}

  SysInt dynamicTriggerCount() {
    return checked_cast<SysInt>(var1.initialMax() - var1.initialMin() + var2.initialMax() -
                                var2.initialMin() + 2);
  }

  virtual void fullPropagate() {
    dvar2 = checked_cast<SysInt>(var1.initialMax() - var1.initialMin() + 1);

    DomainInt maxlim = min(var1.max(), var2.max());
    DomainInt minlim = max(var1.min(), var2.min());
    var1.setMax(maxlim);
    var2.setMax(maxlim);
    var1.setMin(minlim);
    var2.setMin(minlim);

    for(DomainInt val = var1.min(); val <= var1.max(); val++) {
      if(!var2.inDomain(val)) {
        var1.removeFromDomain(val);
      }
    }
    for(DomainInt val = var2.min(); val <= var2.max(); val++) {
      if(!var1.inDomain(val)) {
        var2.removeFromDomain(val);
      }
    }

    for(DomainInt val = var1.min(); val <= var1.max(); val++) {
      if(var1.inDomain(val)) {
        moveTriggerInt(var1, val - var1.initialMin(), DomainRemoval, val);
      }
    }

    for(DomainInt val = var2.min(); val <= var2.max(); val++) {
      if(var2.inDomain(val)) {
        moveTriggerInt(var2, dvar2 + val - var2.initialMin(), DomainRemoval, val);
      }
    }
  }

  virtual void propagateDynInt(SysInt pos, DomainDelta) {
    if(pos < dvar2) {
      DomainInt val = pos + var1.initialMin();
      D_ASSERT(!var1.inDomain(val));
      var2.removeFromDomain(val);
    } else {
      DomainInt val = pos - dvar2 + var2.initialMin();
      D_ASSERT(!var2.inDomain(val));
      var1.removeFromDomain(val);
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 2);
    return (v[0] == v[1]);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(2);
    vars.push_back(var1);
    vars.push_back(var2);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    DomainInt minVal = max(var1.min(), var2.min());
    DomainInt maxVal = min(var1.max(), var2.max());

    for(DomainInt i = minVal; i <= maxVal; ++i) {
      if(var1.inDomain(i) && var2.inDomain(i)) {
        assignment.push_back(make_pair(0, i));
        assignment.push_back(make_pair(1, i));
        return true;
      }
    }
    return false;
  }

  virtual AbstractConstraint* reverseConstraint() {
    return new NeqConstraintBinary<EqualVarRef1, EqualVarRef2>(var1, var2);
  }
};

template <typename T1, typename T2>
AbstractConstraint* BuildCT_GACEQ(const T1& t1, const T2& t2, ConstraintBlob&) {
  return new GACEqualConstraint<typename T1::value_type, typename T2::value_type>(t1[0], t2[0]);
}

/* JSON
{ "type": "constraint",
  "name": "gaceq",
  "internal_name": "CT_GACEQ",
  "args": [ "read_var", "read_var" ]
}
*/
#endif
