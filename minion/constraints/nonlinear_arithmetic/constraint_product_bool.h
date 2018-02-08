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

#ifndef CONSTRAINT_PROD_BOOL_H
#define CONSTRAINT_PROD_BOOL_H


// calculates x*y=z, when x is a boolean.

template <typename ProdVarRef1, typename ProdVarRef2, typename ProdVarRef3>
struct BoolProdConstraint : public AbstractConstraint {
  virtual string constraint_name() {
    return "product";
  }

  ProdVarRef1 var1;
  ProdVarRef2 var2;
  ProdVarRef3 var3;

  CONSTRAINT_ARG_LIST3(var1, var2, var3);

  SysInt dvar3;
  SysInt dvarbool;


  BoolProdConstraint(ProdVarRef1 _v1, ProdVarRef2 _v2, ProdVarRef3 _v3) :
  var1(_v1), var2(_v2), var3(_v3)
  { }
  
  SysInt dynamic_trigger_count() {
    return checked_cast<SysInt>(var2.getInitialMax() - var2.getInitialMin() + var3.getInitialMax() -
                                var3.getInitialMin() + 2 + 3);
  }


  void full_check() {
  if(!var3.inDomain(0)) {
      var1.removeFromDomain(0);
      var2.removeFromDomain(0);
    }

    bool zeroInBool = (var1.getMin() == 0);

    for(DomainInt val = var3.getMin(); val <= var3.getMax(); val++) {
      if(!var2.inDomain(val) && val != 0) {
        var3.removeFromDomain(val);
      }
    }

    if(!var1.inDomain(0) && !var2.inDomain(0)) {
      var3.removeFromDomain(0);
    }

    if(!zeroInBool) {
      for(DomainInt val = var2.getMin(); val <= var2.getMax(); ++val) {
        if(!var3.inDomain(val)) {
          var2.removeFromDomain(val);
        }
      }
    }

    if(var3.isAssignedValue(0) && !var2.inDomain(0)) {
      var1.removeFromDomain(1);
    }

  }

  virtual void full_propagate() {
    dvar3 = checked_cast<SysInt>(var2.getInitialMax() - var2.getInitialMin() + 1);
    dvarbool = dvar3 + checked_cast<SysInt>(var3.getInitialMax() - var3.getInitialMin() + 1);

    full_check();

    for(DomainInt val = var2.getMin(); val <= var2.getMax(); val++) {
      if(var2.inDomain(val)) {
        moveTriggerInt(var2, val - var2.getInitialMin(), DomainRemoval, val);
      }
    }

    for(DomainInt val = var3.getMin(); val <= var3.getMax(); val++) {
      if(var3.inDomain(val)) {
        moveTriggerInt(var3, dvar3 + val - var3.getInitialMin(), DomainRemoval, val);
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
      DomainInt domval = pos + var2.getInitialMin();
      if(domval != 0) {
        var3.removeFromDomain(domval);
      }
      else {
        full_check();
      }
    }
    else if(pos < dvarbool) {
      pos -= dvar3;
      DomainInt domval = pos + var3.getInitialMin();
      if(domval != 0) {
        if(!var3.inDomain(0)) {
          var2.removeFromDomain(domval);
        }
      }
      else {
        full_check();
      }
    }
    else {
      full_check();
    }
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size) {
    D_ASSERT(v_size == 3);
    return (v[0] * v[1] == v[2]);
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> vars;
    vars.reserve(3);
    vars.push_back(var1);
    vars.push_back(var2);
    vars.push_back(var3);
    return vars;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>>& assignment) {
   for(DomainInt v1 = var1.getMin(); v1 <= var1.getMax(); ++v1) {
      if(var1.inDomain(v1)) {
        for(DomainInt v2 = var2.getMin(); v2 <= var2.getMax(); ++v2) {
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
  virtual AbstractConstraint* reverse_constraint() {
    return forward_check_negation(this);
  }
};

#endif
