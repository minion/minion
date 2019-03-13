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

/** @help constraints;w-inrange Description
  The constraint w-inrange(x, [a,b]) ensures that a <= x <= b.
*/

/** @help constraints;w-inrange References
  See also

  help constraints w-notinrange
*/

#ifndef CONSTRAINT_DYNAMIC_UNARY_INRANGE_H
#define CONSTRAINT_DYNAMIC_UNARY_INRANGE_H

// Checks if a variable is in a fixed Range.
template <typename Var>
struct WatchInRangeConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "w-inrange";
  }

  CONSTRAINT_ARG_LIST2(var, make_vec(range_min, range_max));
  Var var;

  DomainInt range_min;
  DomainInt range_max;

  template <typename T>
  WatchInRangeConstraint(const Var& _var, const T& _vals) : var(_var) {
    if(_vals.size() != 2) {
      outputFatalError("The range of an 'inrange' constraint must contain 2 values!");
    }

    range_min = _vals[0];
    range_max = _vals[1];
  }

  virtual SysInt dynamicTriggerCount() {
    return 2;
  }

  virtual void fullPropagate() {
    var.setMin(range_min);
    var.setMax(range_max);
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(WatchInRange);
    D_FATAL_ERROR("Propagation is never called for 'in range'");
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 1);
    return (v[0] >= range_min && v[0] <= range_max);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    /// TODO: Make faster
    DomainInt minVal = max(range_min, var.min());
    DomainInt maxVal = min(range_max, var.max());
    for(DomainInt i = minVal; i <= maxVal; ++i) {
      if(var.inDomain(i)) {
        assignment.push_back(make_pair(0, i));
        return true;
      }
    }
    return false;
  }

  virtual AbstractConstraint* reverseConstraint();
};

// To get reverseConstraint
#include "dynamic_notinrange.h"

template <typename VarArray1>
AbstractConstraint* BuildCT_WATCHED_INRANGE(const VarArray1& _varArray_1,
                                            const ConstraintBlob& b) {
  return new WatchInRangeConstraint<typename VarArray1::value_type>(_varArray_1[0],
                                                                    b.constants[0]);
}

/* JSON
  { "type": "constraint",
    "name": "w-inrange",
    "internal_name": "CT_WATCHED_INRANGE",
    "args": [ "read_var", "read_constant_list" ]
  }
*/
#endif
