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

#ifndef CONSTRAINT_CONSTANT_H
#define CONSTRAINT_CONSTANT_H

template <bool truth>
struct ConstantConstraint : public AbstractConstraint {

  virtual string constraint_name() {
    if (truth)
      return "true";
    else
      return "false";
  }

  CONSTRAINT_ARG_LIST0();

  ConstantConstraint() {}

  virtual triggerCollection setup_internal() {
    triggerCollection t;
    return t;
  }

  virtual void propagateStatic(DomainInt i, DomainDelta) {}

  virtual void full_propagate() {
    if (!truth)
      getState().setFailed(true);
  }

  virtual BOOL check_assignment(DomainInt *v, SysInt v_size) {
    D_ASSERT(v_size == 0);
    return truth;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>> &assignment) { return truth; }

  AbstractConstraint *reverse_constraint() { return new ConstantConstraint<!truth>(); }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> v;
    return v;
  }
};

inline AbstractConstraint *BuildCT_TRUE(ConstraintBlob &) {
  return (new ConstantConstraint<true>());
}

inline AbstractConstraint *BuildCT_FALSE(ConstraintBlob &) {
  return (new ConstantConstraint<false>());
}

/* JSON
{ "type": "constraint",
  "name": "false",
  "internal_name": "CT_FALSE",
  "args": [ ]
}
*/

/* JSON
{ "type": "constraint",
  "name": "true",
  "internal_name": "CT_TRUE",
  "args": [ ]
}
*/

#endif
