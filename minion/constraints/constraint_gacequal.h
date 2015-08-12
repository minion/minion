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

/** @help constraints;eq Description
Constrain two variables to take equal values.
*/

/** @help constraints;eq Example
eq(x0,x1)
*/

/** @help constraints;eq Notes
Achieves bounds consistency.
*/

/** @help constraints;eq Reference
help constraints minuseq
*/

/** @help constraints;minuseq Description
Constraint

   minuseq(x,y)

ensures that x=-y.
*/

/** @help constraints;minuseq Reference
help constraints eq
*/

/** @help constraints;diseq Description
Constrain two variables to take different values.
*/

/** @help constraints;diseq Notes
Achieves arc consistency.
*/

/** @help constraints;diseq Example
diseq(v0,v1)
*/

#ifndef CONSTRAINT_GACEQUAL_H
#define CONSTRAINT_GACEQUAL_H

#include "constraint_equal.h"

template <typename EqualVarRef1, typename EqualVarRef2>
struct GACEqualConstraint : public AbstractConstraint {
  virtual string constraint_name() { return "gaceq"; }

  EqualVarRef1 var1;
  EqualVarRef2 var2;

  CONSTRAINT_ARG_LIST2(var1, var2);

  SysInt dvar2;

  GACEqualConstraint(EqualVarRef1 _var1, EqualVarRef2 _var2) : var1(_var1), var2(_var2) {}

  SysInt dynamic_trigger_count() {
    return checked_cast<SysInt>(var1.getInitialMax() - var1.getInitialMin() + var2.getInitialMax() -
                                var2.getInitialMin() + 2);
  }

  virtual void full_propagate() {
    dvar2 = checked_cast<SysInt>(var1.getInitialMax() - var1.getInitialMin() + 1);

    DomainInt maxlim = min(var1.getMax(), var2.getMax());
    DomainInt minlim = max(var1.getMin(), var2.getMin());
    var1.setMax(maxlim);
    var2.setMax(maxlim);
    var1.setMin(minlim);
    var2.setMin(minlim);

    for (DomainInt val = var1.getMin(); val <= var1.getMax(); val++) {
      if (!var2.inDomain(val)) {
        var1.removeFromDomain(val);
      }
    }
    for (DomainInt val = var2.getMin(); val <= var2.getMax(); val++) {
      if (!var1.inDomain(val)) {
        var2.removeFromDomain(val);
      }
    }

    for (DomainInt val = var1.getMin(); val <= var1.getMax(); val++) {
      if (var1.inDomain(val)) {
        moveTriggerInt(var1, val - var1.getInitialMin(), DomainRemoval, val);
      }
    }

    for (DomainInt val = var2.getMin(); val <= var2.getMax(); val++) {
      if (var2.inDomain(val)) {
        moveTriggerInt(var2, dvar2 + val - var2.getInitialMin(), DomainRemoval, val);
      }
    }
  }

  virtual void propagateDynInt(SysInt pos, DomainDelta) {
    if (pos < dvar2) {
      DomainInt val = pos + var1.getInitialMin();
      D_ASSERT(!var1.inDomain(val));
      var2.removeFromDomain(val);
    } else {
      DomainInt val = pos - dvar2 + var2.getInitialMin();
      D_ASSERT(!var2.inDomain(val));
      var1.removeFromDomain(val);
    }
  }

  virtual BOOL check_assignment(DomainInt *v, SysInt v_size) {
    D_ASSERT(v_size == 2);
    return (v[0] == v[1]);
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> vars;
    vars.reserve(2);
    vars.push_back(var1);
    vars.push_back(var2);
    return vars;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>> &assignment) {
    DomainInt min_val = max(var1.getMin(), var2.getMin());
    DomainInt max_val = min(var1.getMax(), var2.getMax());

    for (DomainInt i = min_val; i <= max_val; ++i) {
      if (var1.inDomain(i) && var2.inDomain(i)) {
        assignment.push_back(make_pair(0, i));
        assignment.push_back(make_pair(1, i));
        return true;
      }
    }
    return false;
  }

  virtual AbstractConstraint *reverse_constraint() {
    return new NeqConstraintBinary<EqualVarRef1, EqualVarRef2>(var1, var2);
  }
};

template <typename T1, typename T2>
AbstractConstraint *BuildCT_GACEQ(const T1 &t1, const T2 &t2, ConstraintBlob &) {
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
