// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

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

  void setupTriggers() {
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
  bool check_modResult(DomainInt i, DomainInt j, DomainInt k) {
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
      if(!check_modResult(b1.min(), b2.min(), b3.min())) {
        getState().setFailed();
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
        getState().setFailed();
        return;
      }
    }

    if(b1.hasSingleValue() && b2.hasSingleValue()) {
      var3.assign(do_mod(b1.min(), b2.min()));
      return;
    }

    if(b2.hasSingleValue() && b3.hasSingleValue() && !var1.isBound()) {
      DomainInt v2 = b2.min();
      DomainInt v3 = b3.min();
      for(DomainInt d = b1.min(); d <= b1.max(); ++d) {
        if(!check_modResult(d,v2,v3)) {
          var1.removeFromDomain(d);
        }
      }
      return;
    }


    if(b2.hasSingleValue()) {
      DomainInt b2_val = b2.min();
      DomainInt divoffset_min = doDiv<undef>(b1.min(), b2_val);
      DomainInt divoffset_max = doDiv<undef>(b1.max(), b2_val);
      if(divoffset_min == divoffset_max) {
        Bounds b = emptyBounds();
        b = addValue(b, do_mod(b1.min(), b2_val));
        b = addValue(b, do_mod(b1.max(), b2_val));
        var3.setMin(b.min());
        var3.setMax(b.max());
        // This could be reasoned slightly more, for more propagation
        // by considering when var3.max()-var3.min() is less than b2_val
        var1.setMin(var3.min() + divoffset_min * b2_val);
        var1.setMax(var3.max() + divoffset_min * b2_val);
      }
    }
  }

  virtual void fullPropagate() {
    setupTriggers();
    propagateDynInt(0, DomainDelta::empty());
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 3);
    return check_modResult(v[0], v[1], v[2]);
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
    return forwardCheckNegation(this);
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
