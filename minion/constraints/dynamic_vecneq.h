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

#include "constraint_checkassign.h"
#include "constraint_equal.h"
#include "constraint_less.h"
#include "dynamic_new_and.h"
#include "nonlinear_arithmetic/constraint_product.h"
#include "unary/dynamic_literal.h"

/** @help constraints;watchvecneq Description
The constraint

   watchvecneq(A, B)

ensures that A and B are not the same vector, i.e., there exists some index i
such that A[i] != B[i].
*/

#ifndef _DYNAMIC_VECNEQ_H
#define _DYNAMIC_VECNEQ_H

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

struct NeqIterated; // because it is used in EqIterated before it is defn

// for the reverse of the hamming constraint:
struct EqIterated {
  static string constraintName() {
    return "not-hamming";
  }

  static SysInt dynamicTriggerCount() {
    return 4;
  }

  static bool checkAssignment(DomainInt i, DomainInt j) {
    return i == j;
  }

  template <typename VarType1, typename VarType2>
  static bool no_support_for_pair(VarType1& var1, VarType2& var2) {
    return var1.min() > var2.max() || var1.max() < var2.min();
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var1(VarType1& var1, VarType2& var2) {
    // just do bounds for the time being
    var2.setMin(var1.min());
    var2.setMax(var1.max());
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var2(VarType1& var1, VarType2& var2) {
    var1.setMin(var2.min());
    var1.setMax(var2.max());
  }

  template <typename VarType1, typename VarType2>
  static void add_triggers(AbstractConstraint* ac, VarType1& var1, VarType2& var2, DomainInt dt) {
    ac->moveTriggerInt(var1, dt, LowerBound);
    ac->moveTriggerInt(var1, dt + 1, UpperBound);
    ac->moveTriggerInt(var2, dt + 2, LowerBound);
    ac->moveTriggerInt(var2, dt + 3, UpperBound);
  }

  template <typename Var1, typename Var2>
  static bool getSatisfyingAssignment(const Var1& var1, const Var2& var2,
                                        pair<DomainInt, DomainInt>& assign) {
    DomainInt min = var1.min();
    if(var2.min() > min)
      min = var2.min();
    DomainInt max = var1.max();
    if(var2.max() < max)
      max = var2.max();
    for(DomainInt i = min; i <= max; i++) {
      if(var1.inDomain(i) && var2.inDomain(i)) {
        assign = make_pair(i, i);
        return true;
      }
    }
    return false;
  }

  template <typename Var1, typename Var2>
  static AbstractConstraint* reverseConstraint(const Var1& var1, const Var2& var2) {
    NeqConstraintBinary<Var1, Var2>* t = new NeqConstraintBinary<Var1, Var2>(var1, var2);
    return (AbstractConstraint*)t;
  }

  typedef NeqIterated reverse_operator;
};

struct NeqIterated {
  static string constraintName() {
    return "hamming";
  }

  static SysInt dynamicTriggerCount() {
    return 2;
  }

  static bool checkAssignment(DomainInt i, DomainInt j) {
    return i != j;
  }

  template <typename VarType1, typename VarType2>
  static bool no_support_for_pair(VarType1& var1, VarType2& var2) {
    return var1.isAssigned() && var2.isAssigned() &&
           var1.assignedValue() == var2.assignedValue();
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var1(VarType1& var1, VarType2& var2) {
    if(var1.isAssigned())
      removeValue(var1.assignedValue(), var2);
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var2(VarType1& var1, VarType2& var2) {
    if(var2.isAssigned())
      removeValue(var2.assignedValue(), var1);
  }

  template <typename VarType1, typename VarType2>
  static void add_triggers(AbstractConstraint* ac, VarType1& var1, VarType2& var2, DomainInt dt) {
    ac->moveTriggerInt(var1, dt, Assigned);
    ac->moveTriggerInt(var2, dt + 1, Assigned);
  }

  template <typename Var>
  static void removeValue(DomainInt val, Var& var) {
    if(var.isBound()) {
      if(var.min() == val)
        var.setMin(val + 1);
      else if(var.max() == val)
        var.setMax(val - 1);
    } else {
      var.removeFromDomain(val);
    }
  }

  template <typename Var1, typename Var2>
  static bool getSatisfyingAssignment(const Var1& var1, const Var2& var2,
                                        pair<DomainInt, DomainInt>& assign) {
    if(var1.isAssigned() && var2.isAssigned() && var1.assignedValue() == var2.assignedValue())
      return false;

    if(var1.isAssigned()) {
      if(var2.min() != var1.assignedValue())
        assign = make_pair(var1.assignedValue(), var2.min());
      else
        assign = make_pair(var1.assignedValue(), var2.max());
    } else {
      if(var1.min() != var2.min())
        assign = make_pair(var1.min(), var2.min());
      else
        assign = make_pair(var1.max(), var2.min());
    }
    return true;
  }

  template <typename Var1, typename Var2>
  static AbstractConstraint* reverseConstraint(const Var1& var1, const Var2& var2) {
    EqualConstraint<Var1, Var2>* t = new EqualConstraint<Var1, Var2>(var1, var2);
    return (AbstractConstraint*)t;
  }

  typedef EqIterated reverse_operator;
};

struct LessIterated {
  static bool checkAssignment(DomainInt i, DomainInt j) {
    return i < j;
  }

  static SysInt dynamicTriggerCount() {
    return 2;
  }

  template <typename VarType1, typename VarType2>
  static bool no_support_for_pair(VarType1& var1, VarType2& var2) {
    return var1.min() >= var2.max();
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var1(VarType1& var1, VarType2& var2) {
    var2.setMin(var1.min() + 1);
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var2(VarType1& var1, VarType2& var2) {
    var1.setMax(var2.max() - 1);
  }

  template <typename VarType1, typename VarType2>
  static void add_triggers(AbstractConstraint* ac, VarType1& var1, VarType2& var2, DomainInt dt) {
    ac->moveTriggerInt(var1, dt, LowerBound);
    ac->moveTriggerInt(var2, dt + 1, UpperBound);
  }

  template <typename Var1, typename Var2>
  static bool getSatisfyingAssignment(const Var1& var1, const Var2& var2,
                                        pair<DomainInt, DomainInt>& assign) {
    if(var1.min() < var2.max()) {
      assign = make_pair(var1.min(), var2.max());
      return true;
    } else
      return false;
  }

  template <typename Var1, typename Var2>
  static AbstractConstraint* reverseConstraint(const Var1& var1, const Var2& var2) {
    LeqConstraint<Var2, Var1, compiletimeVal<SysInt, 0>>* t =
        new LeqConstraint<Var2, Var1, compiletimeVal<SysInt, 0>>(var2, var1,
                                                                  compiletimeVal<SysInt, 0>());
    return (AbstractConstraint*)t;
  }
};

struct BothNonZeroIterated {
  static bool checkAssignment(DomainInt i, DomainInt j) {
    return i > 0 && j > 0;
  }

  static SysInt dynamicTriggerCount() {
    return 2;
  }

  template <typename VarType1, typename VarType2>
  static bool no_support_for_pair(VarType1& var1, VarType2& var2) {
    return var1.max() <= 0 || var2.max() <= 0;
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var1(VarType1& var1, VarType2& var2) {
    var2.setMin(1);
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var2(VarType1& var1, VarType2& var2) {
    var1.setMin(1);
  }

  template <typename VarType1, typename VarType2>
  static void add_triggers(AbstractConstraint* ac, VarType1& var1, VarType2& var2, DomainInt dt) {
    ac->moveTriggerInt(var1, dt, UpperBound);
    ac->moveTriggerInt(var2, dt + 1, UpperBound);
  }

  template <typename Var1, typename Var2>
  static bool getSatisfyingAssignment(const Var1& var1, const Var2& var2,
                                        pair<DomainInt, DomainInt>& assign) {
    if(var1.max() > 0 && var2.max() > 0) {
      assign = make_pair(var1.max(), var2.max());
      return true;
    } else
      return false;
  }

  template <typename Var1, typename Var2>
  static AbstractConstraint* reverseConstraint(const Var1& var1, const Var2& var2) {
    ProductConstraint<Var1, Var2, ConstantVar>* t =
        new ProductConstraint<Var1, Var2, ConstantVar>(var1, var2, ConstantVar(0));
    return (AbstractConstraint*)t;
  }
};

/** Constraints two vectors of variables to be not equal.
 *
 *  \ingroup Constraints
 */
template <typename VarArray1, typename VarArray2, typename Operator = NeqIterated>
struct VecNeqDynamic : public AbstractConstraint {
  virtual string constraintName() {
    return "watchvecneq";
  }

  CONSTRAINT_ARG_LIST2(varArray1, varArray2);

  typedef typename VarArray1::value_type VarRef1;
  typedef typename VarArray2::value_type VarRef2;

  VarArray1 varArray1;
  VarArray2 varArray2;

  SysInt watched_index0;
  SysInt watched_index1;

  Reversible<bool> propagate_mode;
  SysInt indexToPropagate;

  VecNeqDynamic(const VarArray1& _array1, const VarArray2& _array2)
      : varArray1(_array1), varArray2(_array2), propagate_mode(false) {
    D_ASSERT(varArray1.size() == varArray2.size());
  }

  virtual SysInt dynamicTriggerCount() {
    return Operator::dynamicTriggerCount() * 2;
  }

  bool no_support_for_index(SysInt index) {
    return Operator::no_support_for_pair(varArray1[index], varArray2[index]);
  }

  void add_triggers(SysInt index, DomainInt dt) {
    Operator::add_triggers(this, varArray1[index], varArray2[index], dt);
  }

  virtual void fullPropagate() {
    P("VecNeq full prop");
    SysInt size = varArray1.size();
    SysInt index = 0;

    // Find first pair we could watch.
    while(index < size && no_support_for_index(index))
      ++index;

    // Vectors are assigned and equal.
    if(index == size) {
      getState().setFailed(true);
      return;
    }

    watched_index0 = index;

    ++index;

    // Now, is there another fine pair?
    while(index < size && no_support_for_index(index))
      ++index;

    // There is only one possible pair allowed...
    if(index == size) {
      propagate_from_var1(watched_index0);
      propagate_from_var2(watched_index0);
      propagate_mode = true;
      indexToPropagate = watched_index0;
      add_triggers(watched_index0, 0);
      return;
    }

    watched_index1 = index;

    add_triggers(watched_index0, 0);
    add_triggers(watched_index1, 2);
  }

  void propagate_from_var1(SysInt index) {
    Operator::propagate_from_var1(varArray1[index], varArray2[index]);
  }

  void propagate_from_var2(SysInt index) {
    Operator::propagate_from_var2(varArray1[index], varArray2[index]);
  }

  virtual void propagateDynInt(SysInt trigger_activated, DomainDelta) {
    PROP_INFO_ADDONE(DynVecNeq);
    P("VecNeq prop");

    if(propagate_mode) {
      P("Propagating: " << indexToPropagate);
    } else {
      P("Watching " << watched_index0 << "," << watched_index1);
    }

    SysInt triggerpair = trigger_activated / 2;
    D_ASSERT(triggerpair == 0 || triggerpair == 1);
    // Var arrays are numbered 1 and 2
    SysInt triggerarray = (trigger_activated % 2) + 1;
    D_ASSERT(triggerarray == 1 || triggerarray == 2);

    SysInt original_index;
    SysInt other_index;

    if(triggerpair == 0) {
      original_index = watched_index0;
      other_index = watched_index1;
    } else {
      original_index = watched_index1;
      other_index = watched_index0;
    }

    if(propagate_mode) {
      // If this is true, the other index got assigned.
      if(indexToPropagate != original_index)
        return;

      if(triggerarray == 1) {
        propagate_from_var1(indexToPropagate);
      } else {
        propagate_from_var2(indexToPropagate);
      }
      return;
    }

    // Check if propagation has caused a loss of support.
    if(!no_support_for_index(original_index))
      return;

    SysInt index = original_index + 1;

    SysInt size = varArray1.size();

    while((index < size && no_support_for_index(index)) || index == other_index)
      ++index;

    if(index == size) {
      index = 0;
      while((index < original_index && no_support_for_index(index)) || index == other_index)
        ++index;

      if(index == original_index) {
        // This is the only possible non-equal index.
        P("Entering propagate mode for index " << indexToPropagate);
        propagate_mode = true;
        indexToPropagate = other_index;
        propagate_from_var1(other_index);
        propagate_from_var2(other_index);
        return;
      }
    }

    if(triggerpair == 0)
      watched_index0 = index;
    else
      watched_index1 = index;

    D_ASSERT(watched_index0 != watched_index1);
    add_triggers(index, triggerpair * 2);
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    SysInt vSize1 = varArray1.size();
    for(SysInt i = 0; i < vSize1; ++i)
      if(Operator::checkAssignment(v[i], v[i + vSize1]))
        return true;
    return false;
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

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    pair<DomainInt, DomainInt> assign;
    for(SysInt i = 0; i < (SysInt)varArray1.size(); ++i) {
      if(Operator::getSatisfyingAssignment(varArray1[i], varArray2[i], assign)) {
        D_ASSERT(varArray1[i].inDomain(assign.first));
        D_ASSERT(varArray2[i].inDomain(assign.second));
        D_ASSERT(Operator::checkAssignment(assign.first, assign.second));
        assignment.push_back(make_pair(i, assign.first));
        assignment.push_back(make_pair(i + varArray1.size(), assign.second));
        return true;
      }
    }
    return false;
  }

  virtual AbstractConstraint* reverseConstraint() {
    vector<AbstractConstraint*> con;
    for(SysInt i = 0; i < (SysInt)varArray1.size(); i++) {
      con.push_back(Operator::reverseConstraint(varArray1[i], varArray2[i]));
    }
    return new Dynamic_AND(con);
    /*vector<AnyVarRef> t;
    for(SysInt i=0; i<varArray1.size(); i++) t.push_back(varArray1[i]);
    for(SysInt i=0; i<varArray2.size(); i++) t.push_back(varArray2[i]);
    return forward_check_negation(this);*/
  }
};

template <typename VarArray1, typename VarArray2>
AbstractConstraint* BuildCT_WATCHED_VECNEQ(const VarArray1& varray1, const VarArray2& varray2,
                                           ConstraintBlob&) {
  return new VecNeqDynamic<VarArray1, VarArray2>(varray1, varray2);
}

// these two don't seem to be used anywhere
template <typename VarArray1, typename VarArray2>
AbstractConstraint* BuildCT_WATCHED_VEC_OR_LESS(const VarArray1& varray1, const VarArray2& varray2,
                                                ConstraintBlob&) {
  return new VecNeqDynamic<VarArray1, VarArray2, LessIterated>(varray1, varray2);
}

template <typename VarArray1, typename VarArray2>
AbstractConstraint* BuildCT_WATCHED_VEC_OR_AND(const VarArray1& varray1, const VarArray2& varray2,
                                               ConstraintBlob&) {
  return new VecNeqDynamic<VarArray1, VarArray2, BothNonZeroIterated>(varray1, varray2);
}

/* JSON
{ "type": "constraint",
  "name": "watchvecexists_less",
  "internal_name": "CT_WATCHED_VEC_OR_LESS",
  "args": [ "read_list", "read_list" ]
}
*/

/* JSON
{ "type": "constraint",
  "name": "watchvecneq",
  "internal_name": "CT_WATCHED_VECNEQ",
  "args": [ "read_list", "read_list" ]
}
*/

#endif
