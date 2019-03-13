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

  ReversibleInt assignedVars;

  CheckAssignConstraint(const OriginalConstraint& con) : originalcon(con), assignedVars() {}

  virtual AbstractConstraint* reverseConstraint() {
    return new CheckAssignConstraint<OriginalConstraint, !negate>(originalcon);
  }

  virtual void propagateDynInt(SysInt propVal, DomainDelta) {
    PROP_INFO_ADDONE(CheckAssign);
    if(checkUnsat(propVal, DomainDelta::empty()))
      getState().setFailed(true);
  }

  virtual BOOL checkUnsat(SysInt, DomainDelta) {
    typename OriginalConstraint::var_type& variables = originalcon.getVars();

    SysInt count = assignedVars;
    ++count;
    assignedVars = count;
    SysInt vSize = variables.size();
    D_ASSERT(count <= vSize);

    if(count == vSize)
      return checkFullAssignment();
    return false;
  }

  bool checkFullAssignment() {
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

  virtual BOOL fullCheckUnsat() {
    typename OriginalConstraint::var_type& variables = originalcon.getVars();

    UnsignedSysInt counter = 0;
    for(UnsignedSysInt i = 0; i < variables.size(); ++i)
      if(variables[i].isAssigned())
        ++counter;
    assignedVars = counter;

    if(counter == variables.size())
      return checkFullAssignment();
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
    if(fullCheckUnsat())
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

    SysInt freeVar = -1;

    for(UnsignedSysInt i = 0; i < variables.size(); ++i) {
      if(!variables[i].isAssigned()) {
        if(freeVar != -1) {
          D_ASSERT(variables[i].min() != variables[i].max());
          ret_box.push_back(make_pair(i, variables[i].min()));
          ret_box.push_back(make_pair(i, variables[i].max()));
          return true;
        } else {
          freeVar = i;
          c.push_back(-9999); // this value should never be used
        }
      } else
        c.push_back(variables[i].assignedValue());
    }

    if(freeVar == -1) {
      if(try_assignment(ret_box, c))
        return true;
      else
        return false;
    } else {
      D_ASSERT(c[freeVar] == -9999);
      D_ASSERT(variables[freeVar].min() != variables[freeVar].max());

      DomainInt free_min = variables[freeVar].min();
      c[freeVar] = free_min;
      if(try_assignment(ret_box, c))
        return true;
      DomainInt free_max = variables[freeVar].max();
      c[freeVar] = free_max;
      if(try_assignment(ret_box, c))
        return true;

      if(!variables[freeVar].isBound()) {
        for(DomainInt i = variables[freeVar].min() + 1; i < variables[freeVar].max(); ++i) {
          if(variables[freeVar].inDomain(i)) {
            c[freeVar] = i;
            if(try_assignment(ret_box, c))
              return true;
          }
        }
      } else {
        D_ASSERT(free_min != free_max);
        ret_box.push_back(make_pair(freeVar, free_min));
        ret_box.push_back(make_pair(freeVar, free_max));
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

inline AbstractConstraint* forwardCheckNegation(AbstractConstraint* c) {
  return new CheckAssignConstraint<AbstractWrapper>(AbstractWrapper(c));
}
#endif
