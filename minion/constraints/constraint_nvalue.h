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





#ifndef CONSTRAINT_NVALUE_H
#define CONSTRAINT_NVALUE_H

#include "constraint_checkassign.h"
#include <math.h>

template <typename VarArray, typename VarResult>
struct LessEqualNvalueConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "nvalueleq";
  }

  VarArray vars;
  VarResult result;

  CONSTRAINT_ARG_LIST2(vars, result);

  LessEqualNvalueConstraint(VarArray _vars, VarResult _result) : vars(_vars), result(_result) {}

  void triggerSetup() {
    for(unsigned i = 0; i < vars.size(); ++i) {
      moveTriggerInt(vars[i], i, Assigned);
    }

    moveTriggerInt(result, vars.size(), UpperBound);
  }

  virtual SysInt dynamicTriggerCount() {
    return vars.size() + 1;
  }

  virtual void propagateDynInt(SysInt flag, DomainDelta) {
    fullPropagate();
  }

  virtual void fullPropagate() {
    triggerSetup();
    std::set<DomainInt> assigned;
    for(unsigned i = 0; i < vars.size(); ++i) {
      if(vars[i].isAssigned()) {
        assigned.insert(vars[i].assignedValue());
      }
    }

    result.setMin(assigned.size());

    if((DomainInt)assigned.size() == result.max() && assigned.size() > 0) {
      for(unsigned i = 0; i < vars.size(); ++i) {
        if(!vars[i].isAssigned()) {
          vars[i].setMin(*assigned.begin());
          vars[i].setMax(*(--assigned.end()));
          if(!vars[i].isBound()) {
            for(DomainInt d = vars[i].min(); d <= vars[i].max(); ++d) {
              if(assigned.count(d) == 0)
                vars[i].removeFromDomain(d);
            }
          }
        }
      }
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    std::set<DomainInt> assigned;
    for(unsigned i = 0; i < vars.size(); ++i)
      assigned.insert(v[i]);

    return (DomainInt)assigned.size() <= v[vars.size()];
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> v;
    for(unsigned i = 0; i < vars.size(); ++i)
      v.push_back(vars[i]);
    v.push_back(result);
    return v;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(unsigned i = 0; i < vars.size(); ++i) {
      if(vars[i].min() != vars[i].max()) {
        assignment.clear();
        assignment.push_back(make_pair(i, vars[i].min()));
        assignment.push_back(make_pair(i, vars[i].max()));
        return true;
      } else
        assignment.push_back(make_pair(i, vars[i].assignedValue()));
    }

    if(result.min() != result.max()) {
      assignment.clear();
      assignment.push_back(make_pair(vars.size(), result.min()));
      assignment.push_back(make_pair(vars.size(), result.max()));
      return true;
    } else
      assignment.push_back(make_pair(vars.size(), result.assignedValue()));

    std::set<DomainInt> values;
    for(unsigned i = 0; i < vars.size(); ++i)
      values.insert(vars[i].assignedValue());

    return (DomainInt)values.size() <= result.assignedValue();
  }

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverseConstraint() {
    return forwardCheckNegation(this);
  }
};

template <typename VarArray, typename VarResult>
struct GreaterEqualNvalueConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "nvaluegeq";
  }

  VarArray vars;
  VarResult result;

  CONSTRAINT_ARG_LIST2(vars, result);

  GreaterEqualNvalueConstraint(VarArray _vars, VarResult _result) : vars(_vars), result(_result) {}

  virtual SysInt dynamicTriggerCount() {
    return vars.size() + 1;
  }

  void triggerSetup() {
    for(unsigned i = 0; i < vars.size(); ++i) {
      moveTriggerInt(vars[i], i, Assigned);
    }

    moveTriggerInt(result, vars.size(), LowerBound);
  }

  virtual void propagateDynInt(SysInt flag, DomainDelta) {
    propagateImpl();
  }

  virtual void fullPropagate() {
    triggerSetup();
    propagateImpl();
  }

  void propagateImpl() {
    std::set<DomainInt> assigned;
    DomainInt min_unassigned = INT_MAX;
    DomainInt max_unassigned = INT_MIN;
    DomainInt unassignedCount = 0;
    for(unsigned i = 0; i < vars.size(); ++i) {
      if(vars[i].isAssigned()) {
        assigned.insert(vars[i].assignedValue());
      } else {
        unassignedCount++;
        min_unassigned = std::min(min_unassigned, vars[i].min());
        max_unassigned = std::max(max_unassigned, vars[i].max());
      }
    }

    if(unassignedCount == 0) {
      result.setMax(assigned.size());
      return;
    }

    // We do two passes over the domains, for efficiency
    std::set<DomainInt> unassigned_testing;
    std::set<DomainInt> unassigned_appear;
    for(DomainInt i = min_unassigned; i <= max_unassigned; ++i) {
      if(assigned.count(i) == 0)
        unassigned_testing.insert(i);
    }

    for(unsigned i = 0; i < vars.size(); ++i) {
      if(!vars[i].isAssigned()) {
        std::set<DomainInt>::iterator it = unassigned_testing.begin();
        while(it != unassigned_testing.end()) {
          if(vars[i].inDomain(*it)) {
            unassigned_appear.insert(*it);
            std::set<DomainInt>::iterator temp = it;
            ++it;
            unassigned_testing.erase(temp);
          } else {
            ++it;
          }
        }
        if(unassigned_testing.empty())
          break;
      }
    }

    DomainInt unassigned_estimate = std::min(unassignedCount, (DomainInt)unassigned_appear.size());

    result.setMax(assigned.size() + unassigned_estimate);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    std::set<DomainInt> assigned;
    for(unsigned i = 0; i < vars.size(); ++i)
      assigned.insert(v[i]);

    return (DomainInt)assigned.size() >= v[vars.size()];
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> v;
    for(unsigned i = 0; i < vars.size(); ++i)
      v.push_back(vars[i]);
    v.push_back(result);
    return v;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(unsigned i = 0; i < vars.size(); ++i) {
      if(vars[i].min() != vars[i].max()) {
        assignment.clear();
        assignment.push_back(make_pair(i, vars[i].min()));
        assignment.push_back(make_pair(i, vars[i].max()));
        return true;
      } else
        assignment.push_back(make_pair(i, vars[i].assignedValue()));
    }

    if(result.min() != result.max()) {
      assignment.clear();
      assignment.push_back(make_pair(vars.size(), result.min()));
      assignment.push_back(make_pair(vars.size(), result.max()));
      return true;
    } else
      assignment.push_back(make_pair(vars.size(), result.assignedValue()));

    std::set<DomainInt> values;
    for(unsigned i = 0; i < vars.size(); ++i)
      values.insert(vars[i].assignedValue());

    return (DomainInt)values.size() >= result.assignedValue();
  }

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverseConstraint() {
    return forwardCheckNegation(this);
  }
};

template <typename VarArray, typename VarSum>
AbstractConstraint* BuildCT_LEQNVALUE(const VarArray& _varArray, const vector<VarSum>& _varSum,
                                      ConstraintBlob&) {
  return new LessEqualNvalueConstraint<VarArray, VarSum>(_varArray, _varSum[0]);
}

/* JSON
{ "type": "constraint",
  "name": "nvalueleq",
  "internal_name": "CT_LEQNVALUE",
  "args": [ "read_list", "read_var" ]
}
*/

template <typename VarArray, typename VarSum>
AbstractConstraint* BuildCT_GEQNVALUE(const VarArray& _varArray, const vector<VarSum>& _varSum,
                                      ConstraintBlob&) {
  return new GreaterEqualNvalueConstraint<VarArray, VarSum>(_varArray, _varSum[0]);
}

/* JSON
{ "type": "constraint",
  "name": "nvaluegeq",
  "internal_name": "CT_GEQNVALUE",
  "args": [ "read_list", "read_var" ]
}
*/

#endif
