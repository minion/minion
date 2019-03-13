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

/** @help constraints;div Description
The constraint

   div(x,y,z)

ensures that floor(x/y)=z.

For example:

10/3 = 3
(-10)/3 = -4
10/(-3) = -4
(-10)/(-3) = 3

div and mod satisfy together the condition that:

y*(x/y) + x % y = x

The constraint is always false when y = 0
*/

/** @help constraints;div References
help constraints modulo
*/

/** @help constraints;div_undefzero Description
The constraint

   div_undefzero(x,y,z)

is the same as div (it ensures that floor(x/y)=z)
except the constraint is always true when y = 0,
instead of false.

This constraint exists for certain special requirements.
In general, if you are unsure what constraint to use,
then what you want is a plain div constraint!
*/

/** @help constraints;div_undefzero References
help constraints div
*/

#ifndef CONSTRAINT_DIV_H
#define CONSTRAINT_DIV_H

#include "../constraint_checkassign.h"
#include "helper_funcs.h"
#include <math.h>

/// var1 * var2 = var3
template <typename VarRef1, typename VarRef2, typename VarRef3, bool undef>
struct DivConstraint : public AbstractConstraint {
  virtual string constraintName() {
    if(undef)
      return "div_undefzero";
    else
      return "div";
  }

  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;

  CONSTRAINT_ARG_LIST3(var1, var2, var3);

  DivConstraint(VarRef1 _var1, VarRef2 _var2, VarRef3 _var3)
      : var1(_var1), var2(_var2), var3(_var3) {}

  virtual SysInt dynamicTriggerCount() {
    return 6;
  }

  void setupTriggers() {
    moveTriggerInt(var1, 0, LowerBound);
    moveTriggerInt(var1, 1, UpperBound);
    moveTriggerInt(var2, 2, LowerBound);
    moveTriggerInt(var2, 3, UpperBound);
    moveTriggerInt(var3, 4, LowerBound);
    moveTriggerInt(var3, 5, UpperBound);
  }

  // Adds possible values for v1 to Bounds, given v2 and v3
  Bounds addBoundsForVar1(Bounds b, DomainInt v2, DomainInt v3) {
    if(v2 == 0)
      return b;
    b = addValue(b, v2 * v3);
    if(v2 > 0)
      b = addValue(b, v2 * v3 + (v2 - 1));
    else
      b = addValue(b, v2 * v3 + (v2 + 1));
    return b;
  }

  // Adds possible values for v2 to Bounds, given v1 and v3
  Bounds addBoundsForVar2(Bounds b, DomainInt v1, DomainInt v3) {
    if(v3 == 0) {
      if(v1 == 0) {
        b = addValue(b, DomainInt_Min);
        b = addValue(b, DomainInt_Max);
      } else if(v1 > 0) {
        b = addValue(b, v1 + 1);
        b = addValue(b, DomainInt_Max);
      } else if(v1 < 0) {
        b = addValue(b, v1 - 1);
        b = addValue(b, DomainInt_Min);
      }
    } else if(v3 == -1) {
      b = addValue(b, -v1);
      if(v1 > 0) {
        b = addValue(b, DomainInt_Min);
      } else if(v1 < 0) {
        b = addValue(b, DomainInt_Max);
      }
    } else {
      b = addValue(b, do_div<undef>(v1, v3));
      // I think this could be improved slightly, but it's close enough!
      if(v3 != -1)
        b = addValue(b, do_div<undef>(v1 + 1, v3 + 1));
      if(v3 != 1)
        b = addValue(b, do_div<undef>(v1 + 1, v3 - 1));
    }

    return b;
  }

  virtual void propagateDynInt(SysInt, DomainDelta) {
    PROP_INFO_ADDONE(Product);
    Bounds b1 = getBounds(var1);
    Bounds b2 = getBounds(var2);
    Bounds b3 = getBounds(var3);

    int assigedcount = b1.hasSingleValue() + b2.hasSingleValue() + b3.hasSingleValue();

    if(assigedcount == 3) {
      if(!check_div_result<undef>(b1.min(), b2.min(), b3.min())) {
        getState().setFailed(true);
      }
      return;
    }

    // Handle var3:
    {
      Bounds b = emptyBounds();
      if(b2.min() != 0) {
        b = addValue(b, do_div<undef>(b1.min(), b2.min()));
        b = addValue(b, do_div<undef>(b1.max(), b2.min()));
      }
      if(b2.max() != 0) {
        b = addValue(b, do_div<undef>(b1.min(), b2.max()));
        b = addValue(b, do_div<undef>(b1.max(), b2.max()));
      }
      if(b2.contains(1)) {
        b = addValue(b, b1.min());
        b = addValue(b, b1.max());
      }
      if(b2.contains(-1)) {
        b = addValue(b, -b1.min());
        b = addValue(b, -b1.max());
      }

      // Only add if already in domain, to avoid making domain bigger
      if(undef && b2.contains(0) && b3.contains(0)) {
        b = addValue(b, 0);
      }

      var3.setMin(b.min());
      var3.setMax(b.max());
    }

    // Handle var1:
    {
      if(!undef || !b2.contains(0) || !b3.contains(0)) {
        Bounds b = emptyBounds();
        b = addBoundsForVar1(b, b2.min(), b3.min());
        b = addBoundsForVar1(b, b2.min(), b3.max());
        b = addBoundsForVar1(b, b2.max(), b3.min());
        b = addBoundsForVar1(b, b2.max(), b3.max());
        if(b2.contains(1)) {
          b = addValue(b, b3.min());
          b = addValue(b, b3.max());
        }
        if(b2.contains(-1)) {
          b = addValue(b, -b3.min());
          b = addValue(b, -b3.max());
        }
        var1.setMin(b.min());
        var1.setMax(b.max());
      }
    }

    // Handle var2:
    {
      Bounds b = emptyBounds();
      b = addBoundsForVar2(b, b1.min(), b3.min());
      b = addBoundsForVar2(b, b1.min(), b3.max());
      b = addBoundsForVar2(b, b1.max(), b3.min());
      b = addBoundsForVar2(b, b1.max(), b3.max());
      if(b3.contains(-1)) {
        b = addBoundsForVar2(b, b1.min(), -1);
        b = addBoundsForVar2(b, b1.max(), -1);
      }
      if(b3.contains(0)) {
        b = addBoundsForVar2(b, b1.min(), 0);
        b = addBoundsForVar2(b, b1.max(), 0);
      }
      if(undef && b2.contains(0) && b3.contains(0)) {
        b = addValue(b, 0);
      }
      var2.setMin(b.min());
      var2.setMax(b.max());
    }
  }

  virtual void fullPropagate() {
    setupTriggers();
    propagateDynInt(0, DomainDelta::empty());
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 3);
    return check_div_result<undef>(v[0], v[1], v[2]);
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
          if(v2 != 0 && var2.inDomain(v2) && var3.inDomain(do_div<undef>(v1, v2))) {
            assignment.push_back(make_pair(0, v1));
            assignment.push_back(make_pair(1, v2));
            assignment.push_back(make_pair(2, do_div<undef>(v1, v2)));
            return true;
          }
        }
      }
    }
    return false;
  }

  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverseConstraint() {
    return forwardCheckNegation(this);
  }
};

template <typename VarRef1, typename VarRef2>
inline AbstractConstraint* BuildCT_DIV(const vector<VarRef1>& vars, const vector<VarRef2>& var2,
                                       ConstraintBlob&) {
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);

  return new DivConstraint<VarRef1, VarRef1, VarRef2, false>(vars[0], vars[1], var2[0]);
}

/* JSON
{ "type": "constraint",
  "name": "div",
  "internal_name": "CT_DIV",
  "args": [ "read_2_vars", "read_var" ]
}
*/

template <typename VarRef1, typename VarRef2>
inline AbstractConstraint* BuildCT_DIV_UNDEFZERO(const vector<VarRef1>& vars,
                                                 const vector<VarRef2>& var2, ConstraintBlob&) {
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);

  return new DivConstraint<VarRef1, VarRef1, VarRef2, true>(vars[0], vars[1], var2[0]);
}

/* JSON
{ "type": "constraint",
  "name": "div_undefzero",
  "internal_name": "CT_DIV_UNDEFZERO",
  "args": [ "read_2_vars", "read_var" ]
}
*/
#endif
