// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef CONSTRAINT_PROD_BOOL_H
#define CONSTRAINT_PROD_BOOL_H

// calculates x*y=z, when x is a boolean.

template <typename ProdVarRef1, typename ProdVarRef2, typename ProdVarRef3>
struct BoolProdConstraint : public AbstractConstraint {
  virtual string constraintName() {
    return "product";
  }

  ProdVarRef1 var1;
  ProdVarRef2 var2;
  ProdVarRef3 var3;

  CONSTRAINT_ARG_LIST3(var1, var2, var3);

  SysInt dvar3;
  SysInt dvarbool;
  SysInt dvarequalval;

  BoolProdConstraint(ProdVarRef1 _v1, ProdVarRef2 _v2, ProdVarRef3 _v3)
      : var1(_v1), var2(_v2), var3(_v3) {
    if(var1.initialMin() < 0 || var1.initialMax() > 1) {
      D_FATAL_ERROR("Internal error in BoolProdConstraint");
    }
  }

  SysInt dynamicTriggerCount() {
    return checked_cast<SysInt>(var2.initialMax() - var2.initialMin() + 1 +
                                var3.initialMax() - var3.initialMin() + 1 + 2 + 2);
  }

  void fullCheck() {
    if(!var3.inDomain(0)) {
      var1.removeFromDomain(0);
      var2.removeFromDomain(0);
    }

    if(!var1.inDomain(1)) {
      var3.assign(0);
      return;
    }

    if(!var1.inDomain(0) && !var2.inDomain(0)) {
      var3.removeFromDomain(0);
    }

    for(DomainInt val = var3.min(); val <= var3.max(); val++) {
      if(!var2.inDomain(val) && val != 0) {
        var3.removeFromDomain(val);
      }
    }

    if(var1.min() > 0) {
      var2.setMin(var3.min());
      var2.setMax(var3.max());
      for(DomainInt val = var2.min(); val <= var2.max(); ++val) {
        if(!var3.inDomain(val)) {
          var2.removeFromDomain(val);
        }
      }
    }

    find_any_equalValue();
  }

  void find_any_equalValue() {
    if(!var1.inDomain(1)) {
      return;
    }

    DomainInt minval = std::max(var2.min(), var3.min());
    DomainInt maxval = std::min(var2.max(), var3.max());
    for(DomainInt val = minval; val <= maxval; ++val) {
      if(var2.inDomain(val) && var3.inDomain(val)) {
        moveTriggerInt(var2, dvarequalval, DomainRemoval, val);
        moveTriggerInt(var3, dvarequalval + 1, DomainRemoval, val);
      }
      return;
    }
    var1.removeFromDomain(1);
    var3.assign(0);
  }

  virtual void fullPropagate() {
    dvar3 = checked_cast<SysInt>(var2.initialMax() - var2.initialMin() + 1);
    dvarbool = dvar3 + checked_cast<SysInt>(var3.initialMax() - var3.initialMin() + 1);
    dvarequalval = dvarbool + 2;

    fullCheck();

    for(DomainInt val = var2.min(); val <= var2.max(); val++) {
      if(var2.inDomain(val)) {
        moveTriggerInt(var2, val - var2.initialMin(), DomainRemoval, val);
      }
    }

    for(DomainInt val = var3.min(); val <= var3.max(); val++) {
      if(var3.inDomain(val)) {
        moveTriggerInt(var3, dvar3 + val - var3.initialMin(), DomainRemoval, val);
      }
    }

    for(DomainInt val = 0; val <= 1; val++) {
      if(var1.inDomain(val)) {
        moveTriggerInt(var1, dvarbool + val, DomainRemoval, val);
      }
    }
  }

  virtual void propagateDynInt(SysInt pos, DomainDelta) {
    if(pos < dvar3) {
      DomainInt domval = pos + var2.initialMin();
      D_ASSERT(!var2.inDomain(domval));
      if(domval != 0) {
        var3.removeFromDomain(domval);
      } else {
        if(!var1.inDomain(0)) {
          var3.removeFromDomain(0);
        }
      }
    } else if(pos < dvarbool) {
      pos -= dvar3;
      DomainInt domval = pos + var3.initialMin();
      D_ASSERT(!var3.inDomain(domval));
      if(domval != 0) {
        if(var1.min() > 0) {
          var2.removeFromDomain(domval);
        }
      } else {
        var1.removeFromDomain(0);
        var2.removeFromDomain(0);
      }
    } else if(pos < dvarequalval) {
      D_ASSERT(pos == dvarbool || pos == dvarbool + 1);
      if(pos == dvarbool + 1) {
        D_ASSERT(var1.isAssignedValue(0));
        var3.assign(0);
      } else {
        D_ASSERT(var1.isAssignedValue(1));
        // var1 has changed, do a full pass
        fullCheck();
      }
    } else {
      D_ASSERT(pos == dvarequalval || pos == dvarequalval + 1);
      find_any_equalValue();
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == 3);
    return (v[0] * v[1] == v[2]);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(3);
    vars.push_back(var1);
    vars.push_back(var2);
    vars.push_back(var3);
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(DomainInt v1 = var1.min(); v1 <= var1.max(); ++v1) {
      if(var1.inDomain(v1)) {
        for(DomainInt v2 = var2.min(); v2 <= var2.max(); ++v2) {
          if(var2.inDomain(v2) && var3.inDomain(v1 * v2)) {
            assignment.push_back(make_pair(0, v1));
            assignment.push_back(make_pair(1, v2));
            assignment.push_back(make_pair(2, v1 * v2));
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

#endif
