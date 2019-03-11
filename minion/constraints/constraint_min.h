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

/** @help constraints;max Description
The constraint

   max(vec, x)

ensures that x is equal to the maximum value of any variable in vec.
*/

/** @help constraints;max References
See

   help constraints min

for the opposite constraint.
*/

/** @help constraints;min Description
The constraint

   min(vec, x)

ensures that x is equal to the minimum value of any variable in vec.
*/

/** @help constraints;min References
See

   help constraints max

for the opposite constraint.
*/

#ifndef CONSTRAINT_MIN_H
#define CONSTRAINT_MIN_H

#include "constraint_checkassign.h"

template <typename VarArray, typename MinVarRef>
struct MinConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "min";
  }

  virtual string fullOutputName() {
    // We assume constraint is propagated here, we will do a simple check
    // to see if it is true.
    if(min_var.isAssigned()) {
      bool found_assigned_min = false;
      bool found_lesserValue = false;
      for(size_t i = 0; i < var_array.size(); ++i) {
        if(var_array[i].isAssigned() &&
           min_var.assignedValue() == var_array[i].assignedValue())
          found_assigned_min = true;
        if(var_array[i].min() < min_var.min())
          found_lesserValue = true;
      }
      if(found_assigned_min && !found_lesserValue)
        return "true()";
    }

    return ConOutput::print_reversible_con("min", "max", var_array, min_var);
  }

  // typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount>
  // NegConstraintType;
  typedef typename VarArray::value_type ArrayVarRef;

  VarArray var_array;
  MinVarRef min_var;

  MinConstraint(const VarArray& _var_array, const MinVarRef& _min_var)
      : var_array(_var_array), min_var(_min_var) {}

  virtual SysInt dynamicTriggerCount() {
    return (var_array.size() + 1) * 2;
  }

  void setup_triggers() {
    SysInt vSize = var_array.size();
    for(SysInt i = 0; i < vSize; ++i) {
      moveTriggerInt(var_array[i], i, LowerBound);
      moveTriggerInt(var_array[i], i + vSize + 1, UpperBound);
    }
    moveTriggerInt(min_var, vSize, LowerBound);
    moveTriggerInt(min_var, vSize * 2 + 1, UpperBound);
  }

  virtual void propagateDynInt(SysInt propVal, DomainDelta) {
    PROP_INFO_ADDONE(Min);
    SysInt vSize = var_array.size();
    if(propVal <= vSize) { // Lower Bound Changed
      // Had to add 1 to fix "0th array" problem.
      if(propVal == vSize) {
        DomainInt new_min = min_var.min();
        typename VarArray::iterator end = var_array.end();
        for(typename VarArray::iterator it = var_array.begin(); it < end; ++it)
          (*it).setMin(new_min);
      } else {
        typename VarArray::iterator it = var_array.begin();
        typename VarArray::iterator end = var_array.end();
        DomainInt min = it->min();
        ++it;
        for(; it < end; ++it) {
          DomainInt it_min = it->min();
          if(it_min < min)
            min = it_min;
        }
        min_var.setMin(min);
      }
    } else { // Upper Bound Changed
      propVal -= (vSize + 1);
      if(propVal == vSize) {
        typename VarArray::iterator it = var_array.begin();
        DomainInt minvar_max = min_var.max();
        while(it != var_array.end() && (*it).min() > minvar_max)
          ++it;
        if(it == var_array.end()) {
          getState().setFailed(true);
          return;
        }
        // Possibly this variable is the only one that can be the minimum
        typename VarArray::iterator it_copy(it);
        ++it;
        while(it != var_array.end() && (*it).min() > minvar_max)
          ++it;
        if(it != var_array.end()) { // No, another variable can be the minimum
          return;
        }
        it_copy->setMax(minvar_max);
      } else {
        min_var.setMax(var_array[checked_cast<SysInt>(propVal)].max());
      }
    }
  }

  virtual void fullPropagate() {
    setup_triggers();
    SysInt arraySize = var_array.size();
    if(arraySize == 0) {
      getState().setFailed(true);
    } else {
      for(SysInt i = 0; i < (arraySize + 1) * 2; ++i) {
        propagateDynInt(i, DomainDelta::empty());
      }
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)var_array.size() + 1);
    if(vSize == 1)
      return false;

    DomainInt minVal = v[0];
    for(SysInt i = 1; i < vSize - 1; i++)
      minVal = min(minVal, v[i]);
    return minVal == *(v + vSize - 1);
  }

  // Bah: This could be much better!
  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(DomainInt i = min_var.min(); i <= min_var.max(); ++i) {
      if(min_var.inDomain(i)) {
        bool flagDomain = false;
        for(SysInt j = 0; j < (SysInt)var_array.size(); ++j) {
          if(var_array[j].inDomain(i)) {
            flagDomain = true;
            assignment.push_back(make_pair(j, i));
          } else {
            if(var_array[j].max() < i) {
              return false;
            }
            if(var_array[j].initialMin() < i)
              assignment.push_back(make_pair(j, var_array[j].max()));
          }
        }

        if(flagDomain) {
          assignment.push_back(make_pair(var_array.size(), i));
          return true;
        } else
          assignment.clear();
      }
    }
    return false;
  }

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverseConstraint() {
    return forward_check_negation(this);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size() + 1);
    for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    vars.push_back(AnyVarRef(min_var));
    return vars;
  }
};

template <typename VarArray, typename VarRef>
AbstractConstraint* BuildCT_MIN(const VarArray& _var_array, const std::vector<VarRef>& _var_ref,
                                ConstraintBlob&) {
  return (new MinConstraint<VarArray, VarRef>(_var_array, _var_ref[0]));
}

/* JSON
{ "type": "constraint",
  "name": "min",
  "internal_name": "CT_MIN",
  "args": [ "read_list", "read_var" ]
}
*/

template <typename VarArray, typename VarRef>
AbstractConstraint* BuildCT_MAX(const VarArray& _var_array, const vector<VarRef>& _var_ref,
                                ConstraintBlob&) {
  return (new MinConstraint<typename NegType<VarArray>::type, typename NegType<VarRef>::type>(
      VarNegRef(_var_array), VarNegRef(_var_ref[0])));
}

/* JSON
{ "type": "constraint",
  "name": "max",
  "internal_name": "CT_MAX",
  "args": [ "read_list", "read_var" ]
}
*/
#endif
