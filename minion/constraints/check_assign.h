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

#ifndef CHECK_ASSIGN_H_HIUO
#define CHECK_ASSIGN_H_HIUO

#include "../get_info/get_info.h"
#include "../memory_management/reversible_vals.h"
#include "../queue/standard_queue.h"
#include "../triggering/constraint_abstract.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

struct Check_Assign : public AbstractConstraint {

  virtual string extendedName() {
    return constraintName() + ":" + child->extendedName();
  }

  virtual string constraintName() {
    return "check[assign]";
  }

  CONSTRAINT_ARG_LIST1(child);

  AbstractConstraint* child;

  Check_Assign(AbstractConstraint* _con) : child(_con) {}

  virtual AbstractConstraint* reverseConstraint() {
    return new Check_Assign(child->reverseConstraint());
  }

  virtual ~Check_Assign() {
    delete child;
  }

  virtual SysInt dynamicTriggerCount() {
    return 1;
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

  virtual void propagateDynInt(SysInt, DomainDelta) {
    SysInt size = child->getVarsSingleton()->size();
    vector<AnyVarRef>* vars = child->getVarsSingleton();

    for(SysInt i = 0; i < size; ++i) {
      if(!(*vars)[i].isAssigned()) {
        moveTriggerInt((*vars)[i], 0, Assigned);
        return;
      }
    }

    MAKE_STACK_BOX(b, DomainInt, size);
    for(SysInt i = 0; i < size; ++i)
      b.push_back((*vars)[i].assignedValue());

    DomainInt* varptr = 0;
    if(b.size() != 0) {
      varptr = &b[0];
    }

    if(!checkAssignment(varptr, size))
      getState().setFailed(true);
  }

  virtual void fullPropagate() {
    propagateDynInt(0, DomainDelta::empty());
  }
};

inline AbstractConstraint* checkAssignCon(AbstractConstraint* c) {
  return new Check_Assign(c);
}

inline AbstractConstraint* BuildCT_CHECK_ASSIGN(ConstraintBlob& bl) {
  D_ASSERT(bl.internal_constraints.size() == 1);
  return checkAssignCon(build_constraint(bl.internal_constraints[0]));
}

/* JSON
{ "type": "constraint",
  "name": "check[assign]",
  "internal_name": "CT_CHECK_ASSIGN",
  "args": [ "read_constraint" ]
}
*/
#endif
