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
  virtual string constraintName() {
    if(Less)
      return "lexless[quick]";
    else
      return "lexleq[quick]";
  }

  typedef typename VarArray1::value_type VarRef1;
  typedef typename VarArray2::value_type VarRef2;

  VarArray1 varArray1;
  VarArray2 varArray2;

  CONSTRAINT_ARG_LIST2(varArray1, varArray2);

  Reversible<SysInt> alpha;

  QuickLexDynamic(const VarArray1& _array1, const VarArray2& _array2)
      : varArray1(_array1), varArray2(_array2), alpha(-1) {
    CHECK(varArray1.size() == varArray2.size(),
          "QuickLexLeq and QuickLexLess only work with equal length vectors");
  }

  virtual SysInt dynamicTriggerCount() {
    return 2;
  }

  void attachTriggers(SysInt i) {
    P("Attach Trigger: " << i);
    moveTriggerInt(varArray1[i], 0, LowerBound, NoDomainValue, TO_Backtrack);
    moveTriggerInt(varArray2[i], 1, UpperBound, NoDomainValue, TO_Backtrack);
  }

  void detachTriggers() {
    P("Detach Triggers");
    releaseTriggerInt(0, TO_Backtrack);
    releaseTriggerInt(1, TO_Backtrack);
  }

  virtual void fullPropagate() {
    P("Full Prop");

    if(varArray1.size() == 0) {
      if(Less)
        getState().setFailed(true);
      return;
    }

    alpha = 0;

    if(Less && varArray1.size() == 1) {
      varArray2[0].setMin(varArray1[0].min() + 1);
      varArray1[0].setMax(varArray2[0].max() - 1);
    } else {
      varArray2[0].setMin(varArray1[0].min());
      varArray1[0].setMax(varArray2[0].max());
    }

    // Set these up, just so they are stored.
    moveTriggerInt(varArray1[0], 0, LowerBound, NoDomainValue, TO_Store);
    moveTriggerInt(varArray2[0], 1, UpperBound, NoDomainValue, TO_Store);

    if(varArray1[0].isAssigned() && varArray2[0].isAssigned() &&
       varArray1[0].assignedValue() == varArray2[0].assignedValue()) {
      progress();
    }
  }

  void progress() {
    SysInt a = alpha;
    SysInt n = varArray1.size();
    D_ASSERT(varArray1[a].isAssigned());
    D_ASSERT(varArray2[a].isAssigned());
    D_ASSERT(varArray1[a].assignedValue() == varArray2[a].assignedValue());

    a++;

    while(a < n) {
      if(Less && a >= n - 1) {
        varArray2[a].setMin(varArray1[a].min() + 1);
        varArray1[a].setMax(varArray2[a].max() - 1);
      } else {
        varArray1[a].setMax(varArray2[a].max());
        varArray2[a].setMin(varArray1[a].min());
      }

      if(varArray1[a].isAssigned() && varArray2[a].isAssigned() &&
         varArray1[a].assignedValue() == varArray2[a].assignedValue()) {
        a++;
      } else {
        attachTriggers(a);
        alpha = a;
        return;
      }
    }

    if(Less)
      getState().setFailed(true);
    else {
      detachTriggers();
      alpha = n;
    }
  }

  virtual void propagateDynInt(SysInt dt, DomainDelta) {
    SysInt a = alpha;

    if(0 == dt) { // X triggered
      if(Less && a >= (SysInt)varArray1.size() - 1)
        varArray2[a].setMin(varArray1[a].min() + 1);
      else
        varArray2[a].setMin(varArray1[a].min());
    } else { // Y triggered
      if(Less && a >= (SysInt)varArray1.size() - 1)
        varArray1[a].setMax(varArray2[a].max() - 1);
      else
        varArray1[a].setMax(varArray2[a].max());
    }

    if(varArray1[a].isAssigned() && varArray2[a].isAssigned() &&
       varArray1[a].assignedValue() == varArray2[a].assignedValue()) {
      progress();
    } else {
      // if(varArray1[a].max() < varArray2[a].min())
      //    detachTriggers();
    }
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    D_ASSERT(vSize == (SysInt)varArray1.size() + (SysInt)varArray2.size());
    size_t xSize = varArray1.size();

    P("Check Assignment: " << (SysInt)alpha);
    for(size_t i = 0; i < xSize; i++) {
      if(v[i] < v[i + xSize])
        return true;
      if(v[i] > v[i + xSize])
        return false;
    }

    return !Less;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    size_t arraySize = varArray1.size();
    for(size_t i = 0; i < arraySize; ++i) {
      DomainInt x_i_min = varArray1[i].min();
      DomainInt y_iMax = varArray2[i].max();

      if(x_i_min > y_iMax) {
        return false;
      }

      assignment.push_back(make_pair(i, x_i_min));
      assignment.push_back(make_pair(i + arraySize, y_iMax));
      if(x_i_min < y_iMax)
        return true;
    }

    return !Less;
  }

  virtual AbstractConstraint* reverseConstraint() {
    return new QuickLexDynamic<VarArray2, VarArray1, !Less>(varArray2, varArray1);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(varArray1.size() + varArray2.size());
    for(UnsignedSysInt i = 0; i < varArray1.size(); ++i)
      vars.push_back(AnyVarRef(varArray1[i]));
    for(UnsignedSysInt i = 0; i < varArray2.size(); ++i)
      vars.push_back(AnyVarRef(varArray2[i]));
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
