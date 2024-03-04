// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0









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
      bool foundAssigned_min = false;
      bool found_lesserValue = false;
      for(size_t i = 0; i < varArray.size(); ++i) {
        if(varArray[i].isAssigned() &&
           min_var.assignedValue() == varArray[i].assignedValue())
          foundAssigned_min = true;
        if(varArray[i].min() < min_var.min())
          found_lesserValue = true;
      }
      if(foundAssigned_min && !found_lesserValue)
        return "true()";
    }

    return ConOutput::print_reversible_con("min", "max", varArray, min_var);
  }

  // typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount>
  // NegConstraintType;
  typedef typename VarArray::value_type ArrayVarRef;

  VarArray varArray;
  MinVarRef min_var;

  MinConstraint(const VarArray& _varArray, const MinVarRef& _min_var)
      : varArray(_varArray), min_var(_min_var) {}

  virtual SysInt dynamicTriggerCount() {
    return (varArray.size() + 1) * 2;
  }

  void setupTriggers() {
    SysInt vSize = varArray.size();
    for(SysInt i = 0; i < vSize; ++i) {
      moveTriggerInt(varArray[i], i, LowerBound);
      moveTriggerInt(varArray[i], i + vSize + 1, UpperBound);
    }
    moveTriggerInt(min_var, vSize, LowerBound);
    moveTriggerInt(min_var, vSize * 2 + 1, UpperBound);
  }

  virtual void propagateDynInt(SysInt propVal, DomainDelta) {
    PROP_INFO_ADDONE(Min);
    SysInt vSize = varArray.size();
    if(propVal <= vSize) { // Lower Bound Changed
      // Had to add 1 to fix "0th array" problem.
      if(propVal == vSize) {
        DomainInt new_min = min_var.min();
        typename VarArray::iterator end = varArray.end();
        for(typename VarArray::iterator it = varArray.begin(); it < end; ++it)
          (*it).setMin(new_min);
      } else {
        typename VarArray::iterator it = varArray.begin();
        typename VarArray::iterator end = varArray.end();
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
        typename VarArray::iterator it = varArray.begin();
        DomainInt minvarMax = min_var.max();
        while(it != varArray.end() && (*it).min() > minvarMax)
          ++it;
        if(it == varArray.end()) {
          getState().setFailed();
          return;
        }
        // Possibly this variable is the only one that can be the minimum
        typename VarArray::iterator itCopy(it);
        ++it;
        while(it != varArray.end() && (*it).min() > minvarMax)
          ++it;
        if(it != varArray.end()) { // No, another variable can be the minimum
          return;
        }
        itCopy->setMax(minvarMax);
      } else {
        min_var.setMax(varArray[checked_cast<SysInt>(propVal)].max());
      }
    }
  }

  virtual void fullPropagate() {
    setupTriggers();
    SysInt arraySize = varArray.size();
    if(arraySize == 0) {
      getState().setFailed();
    } else {
      for(SysInt i = 0; i < (arraySize + 1) * 2; ++i) {
        propagateDynInt(i, DomainDelta::empty());
      }
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray.size() + 1);
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
        for(SysInt j = 0; j < (SysInt)varArray.size(); ++j) {
          if(varArray[j].inDomain(i)) {
            flagDomain = true;
            assignment.push_back(make_pair(j, i));
          } else {
            if(varArray[j].max() < i) {
              return false;
            }
            if(varArray[j].initialMin() < i)
              assignment.push_back(make_pair(j, varArray[j].max()));
          }
        }

        if(flagDomain) {
          assignment.push_back(make_pair(varArray.size(), i));
          return true;
        } else
          assignment.clear();
      }
    }
    return false;
  }

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverseConstraint() {
    return forwardCheckNegation(this);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray.size() + 1);
    for(UnsignedSysInt i = 0; i < varArray.size(); ++i)
      vars.push_back(AnyVarRef(varArray[i]));
    vars.push_back(AnyVarRef(min_var));
    return vars;
  }
};

template <typename VarArray, typename VarRef>
AbstractConstraint* BuildCT_MIN(const VarArray& _varArray, const std::vector<VarRef>& _var_ref,
                                ConstraintBlob&) {
  return (new MinConstraint<VarArray, VarRef>(_varArray, _var_ref[0]));
}

/* JSON
{ "type": "constraint",
  "name": "min",
  "internal_name": "CT_MIN",
  "args": [ "read_list", "read_var" ]
}
*/

template <typename VarArray, typename VarRef>
AbstractConstraint* BuildCT_MAX(const VarArray& _varArray, const vector<VarRef>& _var_ref,
                                ConstraintBlob&) {
  return (new MinConstraint<typename NegType<VarArray>::type, typename NegType<VarRef>::type>(
      VarNegRef(_varArray), VarNegRef(_var_ref[0])));
}

/* JSON
{ "type": "constraint",
  "name": "max",
  "internal_name": "CT_MAX",
  "args": [ "read_list", "read_var" ]
}
*/
#endif
