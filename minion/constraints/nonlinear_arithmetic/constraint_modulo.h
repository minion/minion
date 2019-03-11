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

/** @help constraints;modulo Description
The constraint

   modulo(x,y,z)

ensures that x%y=z i.e. z is the remainder of dividing x by y.
For negative values, we ensure that:

y(x/y) + x%y = x

To be fully concrete, here are some examples:

3 % 5 = 3
-3 % 5 = 2
3 % -5 = -2
-3 % -5 = -3
*/

/** @help constraints;modulo References
help constraints div
*/

/** @help constraints;mod_undefzero Description
The constraint

   mod_undefzero(x,y,z)

is the same as mod except the constraint is always
true when y = 0, instead of false.

This constraint exists for certain special requirements.
In general, if you are unsure what constraint to use,
then what you want is a plain mod constraint!
*/

/** @help constraints;mod_undefzero References
help constraints mod
*/

#ifndef CONSTRAINT_MODULO_H
#define CONSTRAINT_MODULO_H

#include "../constraint_checkassign.h"
#include "helper_funcs.h"
#include <math.h>

/// var1 % var2 = var3
template <typename VarRef1, typename VarRef2, typename VarRef3, bool undef>
struct ModConstraint : public AbstractConstraint {
  virtual string constraintName() {
    if(undef)
      return "mod_undefzero";
    else
      return "mod";
  }

  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;

  CONSTRAINT_ARG_LIST3(var1, var2, var3);

  ModConstraint(VarRef1 _var1, VarRef2 _var2, VarRef3 _var3)
      : var1(_var1), var2(_var2), var3(_var3) {}

  virtual SysInt dynamicTriggerCount() {
    return 6;
  }

  void setup_triggers() {
    moveTriggerInt(var1, 0, LowerBound);
    moveTriggerInt(var1, 1, UpperBound);
    moveTriggerInt(var2, 2, LowerBound);
    moveTriggerInt(var2, 3, UpperBound);
    moveTriggerInt(var3, 4, LowerBound);
    moveTriggerInt(var3, 5, UpperBound);
  }

  // This function does i%j with Minion semantics.
  // It will assert if undef==false and j==0
  DomainInt do_mod(DomainInt i, DomainInt j) {
    if(j == 0) {
      D_ASSERT(undef);
      return 0;
    }

    // There might well be a slightly better way to do this, but I can't be
    // bothered to figure it out.
    DomainInt r = i % abs(j);
    if(r < 0)
      r += abs(j);
    if(j < 0 && r > 0)
      r -= abs(j);
    return r;
  }

  // This function exists for two reasons:
  // 1) Make sure we never divide by zero
  // 2) Abstract checking mod_undefzero
  bool check_mod_result(DomainInt i, DomainInt j, DomainInt k) {
    if(j == 0) {
      return (undef && k == 0);
    }

    return do_mod(i, j) == k;
  }

  virtual void propagateDynInt(SysInt, DomainDelta) {
    PROP_INFO_ADDONE(Product);
    Bounds b1 = getBounds(var1);
    Bounds b2 = getBounds(var2);
    Bounds b3 = getBounds(var3);

    int assigedcount = b1.hasSingleValue() + b2.hasSingleValue() + b3.hasSingleValue();

    if(assigedcount == 3) {
      if(!check_mod_result(b1.min(), b2.min(), b3.min())) {
        getState().setFailed(true);
      }
      return;
    }

    // Firstly, some simple cases
    if(b2.max() > 0) {
      var3.setMax(b2.max());
    }

    if(b2.min() >= 0) {
      var3.setMin(0);
    }

    if(b2.min() < 0) {
      var3.setMin(b2.min());
    }

    if(b2.max() <= 0) {
      var3.setMax(0);
    }

    if(b2.hasSingleValue() && b2.min() == 0) {
      if(undef) {
        var3.assign(0);
        return;
      } else {
        getState().setFailed(true);
        return;
      }
    }

    if(b1.hasSingleValue() && b2.hasSingleValue()) {
      var3.assign(do_mod(b1.min(), b2.min()));
      return;
    }

    if(b2.hasSingleValue()) {
      if(var3.isBound() || var1.isBound()) {
        if(do_div<undef>(b1.min(), b2.min()) == do_div<undef>(b1.max(), b2.min())) {
          Bounds b = emptyBounds();
          b = addValue(b, do_mod(b1.min(), b2.min()));
          b = addValue(b, do_mod(b1.max(), b2.min()));
          var3.setMin(b.min());
          var3.setMax(b.max());
        }
      } else {
        std::set<DomainInt> vals;
        for(DomainInt d1 = var1.min(); d1 <= var1.max(); ++d1) {
          if(var1.inDomain(d1)) {
            DomainInt modval = do_mod(d1, b2.min());
            if(!var3.inDomain(modval)) {
              var1.removeFromDomain(d1);
            } else {
              vals.insert(modval);
            }
          }
        }

        for(DomainInt d3 = var3.min(); d3 <= var3.max(); ++d3) {
          if(vals.count(d3) == 0) {
            var3.removeFromDomain(d3);
          }
        }
      }
    }
  }

  virtual void fullPropagate() {
    setup_triggers();
    propagateDynInt(0, DomainDelta::empty());
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt v_size) {
    D_ASSERT(v_size == 3);
    return check_mod_result(v[0], v[1], v[2]);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> v;
    v.push_back(var1);
    v.push_back(var2);
    v.push_back(var3);
    return v;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    if(undef) {
      if(var2.inDomain(0) && var3.inDomain(0)) {
        assignment.push_back(make_pair(1, 0));
        assignment.push_back(make_pair(2, 0));
        return true;
      }
    }

    for(DomainInt v1 = var1.min(); v1 <= var1.max(); ++v1) {
      if(var1.inDomain(v1)) {
        for(DomainInt v2 = var2.min(); v2 <= var2.max(); ++v2) {
          if(v2 != 0 && var2.inDomain(v2) && var3.inDomain(do_mod(v1, v2))) {
            assignment.push_back(make_pair(0, v1));
            assignment.push_back(make_pair(1, v2));
            assignment.push_back(make_pair(2, do_mod(v1, v2)));
            return true;
          }
        }
      }
    }
    return false;
  }

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverseConstraint() {
    return forward_check_negation(this);
  }
};

template <typename VarRef1, typename VarRef2>
inline AbstractConstraint* BuildCT_MODULO(const vector<VarRef1>& vars, const vector<VarRef2>& var2,
                                          ConstraintBlob&) {
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);

  return new ModConstraint<VarRef1, VarRef1, VarRef2, false>(vars[0], vars[1], var2[0]);
}
/* JSON
{ "type": "constraint",
  "name": "modulo",
  "internal_name": "CT_MODULO",
  "args": [ "read_2_vars", "read_var" ]
}
*/

template <typename VarRef1, typename VarRef2>
inline AbstractConstraint* BuildCT_MODULO_UNDEFZERO(const vector<VarRef1>& vars,
                                                    const vector<VarRef2>& var2, ConstraintBlob&) {
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);

  return new ModConstraint<VarRef1, VarRef1, VarRef2, true>(vars[0], vars[1], var2[0]);
}

/* JSON
{ "type": "constraint",
  "name": "modulo_undefzero",
  "internal_name": "CT_MODULO_UNDEFZERO",
  "args": [ "read_2_vars", "read_var" ]
}
*/

#endif
