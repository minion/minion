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

/** @help constraints;alldiff Description
Forces the input vector of variables to take distinct values.
*/

/** @help constraints;alldiff Example
Suppose the input file had the following vector of variables defined:

DISCRETE myVec[9] {1..9}

To ensure that each variable takes a different value include the
following constraint:

alldiff(myVec)
*/

/** @help constraints;alldiff Notes
Enforces the same level of consistency as a clique of not equals
constraints.
*/

/** @help constraints;alldiff References
See

   help constraints gacalldiff

for the same constraint that enforces GAC.
*/

#ifndef CONSTRAINT_NEQ_H
#define CONSTRAINT_NEQ_H

#include "constraint_checkassign.h"

template <typename VarArray>
struct NeqConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "alldiff";
  }

  // typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount>
  // NegConstraintType;
  typedef typename VarArray::value_type VarRef;

  VarArray var_array;

  CONSTRAINT_ARG_LIST1(var_array);

  NeqConstraint(const VarArray& _var_array) : var_array(_var_array) {}

  virtual SysInt dynamicTriggerCount() {
    return var_array.size();
  }

  void setup_triggers() {
    for(SysInt i = 0; i < var_array.size(); ++i)
      moveTriggerInt(var_array[i], i, Assigned);
  }

  virtual AbstractConstraint* reverseConstraint() {
    return forward_check_negation(this);
  }

  virtual void propagateDynInt(SysInt propVal_in, DomainDelta) {
    const SysInt propVal = checked_cast<SysInt>(propVal_in);
    PROP_INFO_ADDONE(ArrayNeq);
    DomainInt removeVal = var_array[propVal].assignedValue();
    SysInt arraySize = var_array.size();
    for(SysInt i = 0; i < arraySize; ++i) {
      if(i != propVal) {
        if(var_array[i].isBound()) {
          if(var_array[i].min() == removeVal)
            var_array[i].setMin(removeVal + 1);
          if(var_array[i].max() == removeVal)
            var_array[i].setMax(removeVal - 1);
        } else {
          var_array[i].removeFromDomain(removeVal);
        }
      }
    }
  }

  virtual void fullPropagate() {
    setup_triggers();
    SysInt arraySize = var_array.size();
    for(SysInt i = 0; i < arraySize; ++i)
      if(var_array[i].isAssigned()) {
        DomainInt removeVal = var_array[i].assignedValue();
        for(SysInt j = 0; j < arraySize; ++j) {
          if(i != j) {
            if(var_array[j].isBound()) {
              if(var_array[j].min() == removeVal)
                var_array[j].setMin(removeVal + 1);
              if(var_array[j].max() == removeVal)
                var_array[j].setMax(removeVal - 1);
            } else {
              var_array[j].removeFromDomain(removeVal);
            }
          }
        }
      }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)var_array.size());
    SysInt arraySize = checked_cast<SysInt>(vSize);
    for(SysInt i = 0; i < arraySize; i++)
      for(SysInt j = i + 1; j < arraySize; j++)
        if(v[i] == v[j])
          return false;
    return true;
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size());
    for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
      vars.push_back(var_array[i]);
    return vars;
  }

  // Getting a satisfying assignment here is too hard, we don't want to have to
  // build a matching.
  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    MAKE_STACK_BOX(c, DomainInt, var_array.size());

    for(UnsignedSysInt i = 0; i < var_array.size(); ++i) {
      if(!var_array[i].isAssigned()) {
        assignment.push_back(make_pair(i, var_array[i].min()));
        assignment.push_back(make_pair(i, var_array[i].max()));
        return true;
      } else
        c.push_back(var_array[i].assignedValue());
    }

    if(checkAssignment(c.begin(), c.size())) { // Put the complete assignment in the box.
      for(SysInt i = 0; i < (SysInt)var_array.size(); ++i)
        assignment.push_back(make_pair(i, c[i]));
      return true;
    }
    return false;
  }
};

template <typename VarArray>
AbstractConstraint* BuildCT_ALLDIFF(const VarArray& var_array, ConstraintBlob&) {
  return new NeqConstraint<VarArray>(var_array);
}

/* JSON
{ "type": "constraint",
  "name": "alldiff",
  "internal_name": "CT_ALLDIFF",
  "args": [ "read_list" ]
}
*/

#endif
