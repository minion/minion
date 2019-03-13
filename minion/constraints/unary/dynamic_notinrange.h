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

/** @help constraints;w-notinrange Description
  The constraint w-notinrange(x, [a,b]) ensures that x < a or b < x.
*/

/** @help constraints;w-notinrange References
  See also

  help constraints w-inrange
*/

#ifndef CONSTRAINT_DYNAMIC_UNARY_NOTINRANGE_H
#define CONSTRAINT_DYNAMIC_UNARY_NOTINRANGE_H

#include "dynamic_inrange.h"

// Checks if a variable is in a fixed Range.
template <typename Var>
struct WatchNotInRangeConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "w-notinrange";
  }

  CONSTRAINT_ARG_LIST2(var, make_vec(rangeMin, rangeMax));
  Var var;

  DomainInt rangeMin;
  DomainInt rangeMax;

  template <typename T>
  WatchNotInRangeConstraint(const Var& _var, const T& _vals) : var(_var) {
    if(_vals.size() != 2) {
      cerr << "The range of an 'NotInRange' constraint must contain 2 values!" << endl;
      abort();
    }

    rangeMin = _vals[0];
    rangeMax = _vals[1];
  }

  virtual SysInt dynamicTriggerCount() {
    return 2;
  }

  virtual void fullPropagate() {
    // Ignore empty ranges
    if(rangeMin > rangeMax)
      return;

    if(var.max() <= rangeMax) {
      var.setMax(rangeMin - 1);
      return;
    }

    if(var.min() >= rangeMin) {
      var.setMin(rangeMax + 1);
      return;
    }

    if(var.isBound()) {
      moveTriggerInt(var, 0, DomainChanged);
      propagateDynInt(0, DomainDelta::empty());
    } else {
      for(DomainInt i = rangeMin; i <= rangeMax; ++i)
        var.removeFromDomain(i);
    }
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    PROP_INFO_ADDONE(WatchNotInRange);
    D_ASSERT(var.isBound());

    if(var.max() <= rangeMax) {
      var.setMax(rangeMin - 1);
      return;
    }

    if(var.min() >= rangeMin) {
      var.setMin(rangeMax + 1);
      return;
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 1);
    return (v[0] < rangeMin || v[0] > rangeMax);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    /// TODO: Make faster
    if(var.min() < rangeMin || var.min() > rangeMax) {
      assignment.push_back(make_pair(0, var.min()));
      return true;
    }

    if(var.max() < rangeMin || var.max() > rangeMax) {
      assignment.push_back(make_pair(0, var.max()));
      return true;
    }
    return false;
  }

  virtual AbstractConstraint* reverseConstraint() {
    std::array<DomainInt, 2> a = {{rangeMin, rangeMax}};
    return new WatchInRangeConstraint<Var>(var, a);
  }
};

// From dynamic_inrange.h
template <typename Var>
AbstractConstraint* WatchInRangeConstraint<Var>::reverseConstraint() {
  std::array<DomainInt, 2> a = {{rangeMin, rangeMax}};
  return new WatchNotInRangeConstraint<Var>(var, a);
}

template <typename VarArray1>
AbstractConstraint* BuildCT_WATCHED_NOT_INRANGE(const VarArray1& _varArray_1,
                                                const ConstraintBlob& b) {
  return new WatchNotInRangeConstraint<typename VarArray1::value_type>(_varArray_1[0],
                                                                       b.constants[0]);
}

/* JSON
  { "type": "constraint",
    "name": "w-notinrange",
    "internal_name": "CT_WATCHED_NOT_INRANGE",
    "args": [ "read_var", "read_constant_list" ]
  }
*/

#endif
