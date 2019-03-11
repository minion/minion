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

/** @help constraints;ineq Description
The constraint

   ineq(x, y, k)

ensures that

   x <= y + k

in any solution.
*/

/** @help constraints;ineq Notes
Minion has no strict inequality (<) constraints. However x < y can be
achieved by

   ineq(x, y, -1)
*/

#ifndef CONSTRAINT_LESS_H
#define CONSTRAINT_LESS_H

// x <= y + offset
template <typename VarRef1, typename VarRef2, typename Offset>
struct LeqConstraint : public AbstractConstraint {
  virtual string constraint_name() {
    return "ineq";
  }

  // typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount>
  // NegConstraintType;
  VarRef1 x;
  VarRef2 y;
  const Offset offset;

  CONSTRAINT_ARG_LIST3(x, y, offset);

  LeqConstraint(VarRef1 _x, VarRef2 _y, Offset _o) : x(_x), y(_y), offset(_o) {}

  virtual SysInt dynamic_trigger_count() {
    return 2;
  }

  // Needs to be at end of file
  virtual AbstractConstraint* reverse_constraint();

  virtual void propagateDynInt(SysInt prop_val, DomainDelta) {
    PROP_INFO_ADDONE(BinaryLeq);
    if(checked_cast<SysInt>(prop_val)) { // y changed
      x.setMax(y.max() + offset);
    } else { // x changed
      y.setMin(x.min() - offset);
    }
  }

  virtual void full_propagate() {
    moveTriggerInt(x, 0, LowerBound);
    moveTriggerInt(y, 1, UpperBound);

    propagateDynInt(0, DomainDelta::empty());
    propagateDynInt(1, DomainDelta::empty());
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size) {
    D_ASSERT(v_size == 2);
    return v[0] <= (v[1] + offset);
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>>& assignment) {
    DomainInt x_min = x.min();
    DomainInt y_max = y.max();

    if(x_min <= y_max + offset) {
      assignment.push_back(make_pair(0, x_min));
      assignment.push_back(make_pair(1, y_max));
      return true;
    }
    return false;
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> array;
    array.reserve(2);
    array.push_back(x);
    array.push_back(y);
    return array;
  }
};

template <typename VarRef1, typename VarRef2, typename Offset>
AbstractConstraint* LeqCon(VarRef1 v1, VarRef2 v2, Offset o) {
  return new LeqConstraint<VarRef1, VarRef2, Offset>(v1, v2, o);
}

template <typename VarRef1, typename VarRef2>
AbstractConstraint* LeqCon(VarRef1 v1, VarRef2 v2) {
  return new LeqConstraint<VarRef1, VarRef2, compiletime_val<SysInt, 0>>(
      v1, v2, compiletime_val<SysInt, 0>());
}

template <typename VarRef>
AbstractConstraint* ImpliesCon(VarRef v1, VarRef v2) {
  return new LeqConstraint<VarRef, VarRef, compiletime_val<SysInt, 0>>(
      v1, v2, compiletime_val<SysInt, 0>());
}

// This is mainly inline to avoid multiple definitions.
template <typename VarRef1, typename VarRef2, typename Offset>
inline AbstractConstraint* LeqConstraint<VarRef1, VarRef2, Offset>::reverse_constraint() {
  return LeqCon(y, x, const_negminusone(offset));
}

template <typename T1, typename T2>
AbstractConstraint* BuildCT_INEQ(const T1& t1, const T2& t2, ConstraintBlob& b) {
  return LeqCon(t1[0], t2[0], b.constants[0][0]);
}

/* JSON
{ "type": "constraint",
  "name": "ineq",
  "internal_name": "CT_INEQ",
  "args": [ "read_var", "read_var", "read_constant" ]
}
*/

#endif
