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

#ifndef DYNAMIC_QUICKLEX_H
#define DYNAMIC_QUICKLEX_H

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

template <typename VarArray1, typename VarArray2, bool Less = false>
struct QuickLexDynamic : public AbstractConstraint {
  virtual string constraint_name() {
    if(Less)
      return "lexless[quick]";
    else
      return "lexleq[quick]";
  }

  typedef typename VarArray1::value_type VarRef1;
  typedef typename VarArray2::value_type VarRef2;

  VarArray1 var_array1;
  VarArray2 var_array2;

  CONSTRAINT_ARG_LIST2(var_array1, var_array2);

  Reversible<SysInt> alpha;

  QuickLexDynamic(const VarArray1& _array1, const VarArray2& _array2)
      : var_array1(_array1), var_array2(_array2), alpha(-1) {
    CHECK(var_array1.size() == var_array2.size(),
          "QuickLexLeq and QuickLexLess only work with equal length vectors");
  }

  virtual SysInt dynamic_trigger_count() {
    return 2;
  }

  void attach_triggers(SysInt i) {
    P("Attach Trigger: " << i);
    moveTriggerInt(var_array1[i], 0, LowerBound, NoDomainValue, TO_Backtrack);
    moveTriggerInt(var_array2[i], 1, UpperBound, NoDomainValue, TO_Backtrack);
  }

  void detach_triggers() {
    P("Detach Triggers");
    releaseTriggerInt(0, TO_Backtrack);
    releaseTriggerInt(1, TO_Backtrack);
  }

  virtual void full_propagate() {
    P("Full Prop");

    if(var_array1.size() == 0) {
      if(Less)
        getState().setFailed(true);
      return;
    }

    alpha = 0;

    if(Less && var_array1.size() == 1) {
      var_array2[0].setMin(var_array1[0].min() + 1);
      var_array1[0].setMax(var_array2[0].max() - 1);
    } else {
      var_array2[0].setMin(var_array1[0].min());
      var_array1[0].setMax(var_array2[0].max());
    }

    // Set these up, just so they are stored.
    moveTriggerInt(var_array1[0], 0, LowerBound, NoDomainValue, TO_Store);
    moveTriggerInt(var_array2[0], 1, UpperBound, NoDomainValue, TO_Store);

    if(var_array1[0].isAssigned() && var_array2[0].isAssigned() &&
       var_array1[0].assignedValue() == var_array2[0].assignedValue()) {
      progress();
    }
  }

  void progress() {
    SysInt a = alpha;
    SysInt n = var_array1.size();
    D_ASSERT(var_array1[a].isAssigned());
    D_ASSERT(var_array2[a].isAssigned());
    D_ASSERT(var_array1[a].assignedValue() == var_array2[a].assignedValue());

    a++;

    while(a < n) {
      if(Less && a >= n - 1) {
        var_array2[a].setMin(var_array1[a].min() + 1);
        var_array1[a].setMax(var_array2[a].max() - 1);
      } else {
        var_array1[a].setMax(var_array2[a].max());
        var_array2[a].setMin(var_array1[a].min());
      }

      if(var_array1[a].isAssigned() && var_array2[a].isAssigned() &&
         var_array1[a].assignedValue() == var_array2[a].assignedValue()) {
        a++;
      } else {
        attach_triggers(a);
        alpha = a;
        return;
      }
    }

    if(Less)
      getState().setFailed(true);
    else {
      detach_triggers();
      alpha = n;
    }
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    SysInt a = alpha;

    if(0 == dt) { // X triggered
      if(Less && a >= (SysInt)var_array1.size() - 1)
        var_array2[a].setMin(var_array1[a].min() + 1);
      else
        var_array2[a].setMin(var_array1[a].min());
    } else { // Y triggered
      if(Less && a >= (SysInt)var_array1.size() - 1)
        var_array1[a].setMax(var_array2[a].max() - 1);
      else
        var_array1[a].setMax(var_array2[a].max());
    }

    if(var_array1[a].isAssigned() && var_array2[a].isAssigned() &&
       var_array1[a].assignedValue() == var_array2[a].assignedValue()) {
      progress();
    } else {
      // if(var_array1[a].max() < var_array2[a].min())
      //    detach_triggers();
    }
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size) {
    D_ASSERT(v_size == (SysInt)var_array1.size() + (SysInt)var_array2.size());
    size_t x_size = var_array1.size();

    P("Check Assignment: " << (SysInt)alpha);
    for(size_t i = 0; i < x_size; i++) {
      if(v[i] < v[i + x_size])
        return true;
      if(v[i] > v[i + x_size])
        return false;
    }

    return !Less;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>>& assignment) {
    size_t array_size = var_array1.size();
    for(size_t i = 0; i < array_size; ++i) {
      DomainInt x_i_min = var_array1[i].min();
      DomainInt y_i_max = var_array2[i].max();

      if(x_i_min > y_i_max) {
        return false;
      }

      assignment.push_back(make_pair(i, x_i_min));
      assignment.push_back(make_pair(i + array_size, y_i_max));
      if(x_i_min < y_i_max)
        return true;
    }

    return !Less;
  }

  virtual AbstractConstraint* reverse_constraint() {
    return new QuickLexDynamic<VarArray2, VarArray1, !Less>(var_array2, var_array1);
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> vars;
    vars.reserve(var_array1.size() + var_array2.size());
    for(UnsignedSysInt i = 0; i < var_array1.size(); ++i)
      vars.push_back(AnyVarRef(var_array1[i]));
    for(UnsignedSysInt i = 0; i < var_array2.size(); ++i)
      vars.push_back(AnyVarRef(var_array2[i]));
    return vars;
  }
};

template <typename VarArray1, typename VarArray2>
AbstractConstraint* BuildCT_QUICK_LEXLEQ(const VarArray1& x, const VarArray2& y, ConstraintBlob&) {
  return new QuickLexDynamic<VarArray1, VarArray2, false>(x, y);
}

template <typename VarArray1, typename VarArray2>
AbstractConstraint* BuildCT_QUICK_LEXLESS(const VarArray1& x, const VarArray2& y, ConstraintBlob&) {
  return new QuickLexDynamic<VarArray1, VarArray2, true>(x, y);
}

/* JSON
{ "type": "constraint",
  "name": "lexleq[quick]",
  "internal_name": "CT_QUICK_LEXLEQ",
  "args": [ "read_list", "read_list" ]
}
*/

/* JSON
{ "type": "constraint",
  "name": "lexless[quick]",
  "internal_name": "CT_QUICK_LEXLESS",
  "args": [ "read_list", "read_list" ]
}
*/

#endif
