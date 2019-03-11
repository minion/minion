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

// This is a standard fall-back constraint which can be used when a
// constraint has not got a better method of implementing their
// inverse for reification.

#ifndef CONSTRAINT_CHECKASSIGN_H
#define CONSTRAINT_CHECKASSIGN_H

template <typename OriginalConstraint, bool negate = true>
struct CheckAssignConstraint : public AbstractConstraint {
  virtual string constraintName() {
    if(negate)
      return "!" + originalcon.constraintName();
    return originalcon.constraintName();
  }

  virtual string fullOutputName() {
    return originalcon.fullOutputName();
  }

  OriginalConstraint originalcon;

  ReversibleInt assigned_vars;

  CheckAssignConstraint(const OriginalConstraint& con) : originalcon(con), assigned_vars() {}

  virtual AbstractConstraint* reverseConstraint() {
    return new CheckAssignConstraint<OriginalConstraint, !negate>(originalcon);
  }

  virtual void propagateDynInt(SysInt propVal, DomainDelta) {
    PROP_INFO_ADDONE(CheckAssign);
    if(check_unsat(propVal, DomainDelta::empty()))
      getState().setFailed(true);
  }

  virtual BOOL check_unsat(SysInt, DomainDelta) {
    typename OriginalConstraint::var_type& variables = originalcon.getVars();

    SysInt count = assigned_vars;
    ++count;
    assigned_vars = count;
    SysInt vSize = variables.size();
    D_ASSERT(count <= vSize);

    if(count == vSize)
      return check_full_assignment();
    return false;
  }

  bool check_full_assignment() {
    typename OriginalConstraint::var_type& variables = originalcon.getVars();

    MAKE_STACK_BOX(assignment, DomainInt, variables.size());
    for(UnsignedSysInt i = 0; i < variables.size(); ++i) {
      D_ASSERT(variables[i].isAssigned());
      assignment.push_back(variables[i].assignedValue());
    }
    if(assignment.size() == 0)
      return !checkAssignment(NULL, 0);
    else
      return !checkAssignment(&assignment.front(), assignment.size());
  }

  virtual BOOL full_check_unsat() {
    typename OriginalConstraint::var_type& variables = originalcon.getVars();

    UnsignedSysInt counter = 0;
    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      if(variables[i].isAssigned())
        ++counter;
    assigned_vars = counter;

    if(counter == variables.size())
      return check_full_assignment();
    return false;
  }

  virtual SysInt dynamicTriggerCount() {
    typename OriginalConstraint::var_type& variables = originalcon.getVars();
    return variables.size();
  }

  void trigger_setup() {
    typename OriginalConstraint::var_type& variables = originalcon.getVars();
    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      moveTriggerInt(variables[i], i, Assigned);
  }

  virtual void fullPropagate() {
    trigger_setup();
    if(full_check_unsat())
      getState().setFailed(true);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    typename OriginalConstraint::var_type& variables = originalcon.getVars();
    (void)variables;
    D_ASSERT(vSize == (SysInt)variables.size());
    if(negate)
      return !originalcon.checkAssignment(v, vSize);
    else
      return originalcon.checkAssignment(v, vSize);
  }

  virtual vector<AnyVarRef> getVars() {
    typename OriginalConstraint::var_type& variables = originalcon.getVars();

    vector<AnyVarRef> vars;
    vars.reserve(variables.size());
    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      vars.push_back(variables[i]);
    return vars;
  }

  // Getting a satisfying assignment here is too hard.
  // Let's at least try forward checking!
  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& ret_box) {
    typename OriginalConstraint::var_type& variables = originalcon.getVars();

    MAKE_STACK_BOX(c, DomainInt, variables.size());
    D_ASSERT(variables.size() == originalcon.getVars().size());
    D_ASSERT(ret_box.size() == 0);

    SysInt free_var = -1;

    for(UnsignedSysInt i = 0; i < variables.size(); ++i) {
      if(!variables[i].isAssigned()) {
        if(free_var != -1) {
          D_ASSERT(variables[i].min() != variables[i].max());
          ret_box.push_back(make_pair(i, variables[i].min()));
          ret_box.push_back(make_pair(i, variables[i].max()));
          return true;
        } else {
          free_var = i;
          c.push_back(-9999); // this value should never be used
        }
      } else
        c.push_back(variables[i].assignedValue());
    }

    if(free_var == -1) {
      if(try_assignment(ret_box, c))
        return true;
      else
        return false;
    } else {
      D_ASSERT(c[free_var] == -9999);
      D_ASSERT(variables[free_var].min() != variables[free_var].max());

      DomainInt free_min = variables[free_var].min();
      c[free_var] = free_min;
      if(try_assignment(ret_box, c))
        return true;
      DomainInt free_max = variables[free_var].max();
      c[free_var] = free_max;
      if(try_assignment(ret_box, c))
        return true;

      if(!variables[free_var].isBound()) {
        for(DomainInt i = variables[free_var].min() + 1; i < variables[free_var].max(); ++i) {
          if(variables[free_var].inDomain(i)) {
            c[free_var] = i;
            if(try_assignment(ret_box, c))
              return true;
          }
        }
      } else {
        D_ASSERT(free_min != free_max);
        ret_box.push_back(make_pair(free_var, free_min));
        ret_box.push_back(make_pair(free_var, free_max));
        return true;
      }
    }
    return false;
  }

  template <typename Box, typename Check>
  bool try_assignment(Box& assign, Check& check) {
    if(checkAssignment(check.begin(), check.size())) {
      // Put the complete assignment in the box.
      for(SysInt i = 0; i < (SysInt)check.size(); ++i)
        assign.push_back(make_pair(i, check[i]));
      return true;
    } else
      return false;
  }
};

class AbstractWrapper {
  AbstractConstraint* c;

public:
  typedef vector<AnyVarRef> var_type;

  AbstractWrapper(AbstractConstraint* _c) : c(_c) {}

  string fullOutputName() const {
    return "";
  }

  string constraintName() const {
    return c->constraintName();
  }

  vector<AnyVarRef>& getVars() {
    return *(c->getVarsSingleton());
  }

  virtual bool checkAssignment(DomainInt* v, SysInt vSize) {
    return c->checkAssignment(v, vSize);
  }
};

inline AbstractConstraint* forward_check_negation(AbstractConstraint* c) {
  return new CheckAssignConstraint<AbstractWrapper>(AbstractWrapper(c));
}
#endif
