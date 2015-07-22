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

#include "dynamic_new_and.h"
#include "unary/dynamic_literal.h"
#include "constraint_less.h"
#include "constraint_equal.h"
#include "constraint_product.h"
#include "constraint_checkassign.h"

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
  static string constraint_name() { return "not-hamming"; }

  static SysInt dynamic_trigger_count() { return 4; }

  static bool check_assignment(DomainInt i, DomainInt j) { return i == j; }

  template <typename VarType1, typename VarType2>
  static bool no_support_for_pair(VarType1 &var1, VarType2 &var2) {
    return var1.getMin() > var2.getMax() || var1.getMax() < var2.getMin();
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var1(VarType1 &var1, VarType2 &var2) {
    // just do bounds for the time being
    var2.setMin(var1.getMin());
    var2.setMax(var1.getMax());
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var2(VarType1 &var1, VarType2 &var2) {
    var1.setMin(var2.getMin());
    var1.setMax(var2.getMax());
  }

  template <typename VarType1, typename VarType2>
  static void add_triggers(AbstractConstraint *ac, VarType1 &var1, VarType2 &var2, DomainInt dt) {
    ac->moveTriggerInt(var1, dt, LowerBound);
    ac->moveTriggerInt(var1, dt + 1, UpperBound);
    ac->moveTriggerInt(var2, dt + 2, LowerBound);
    ac->moveTriggerInt(var2, dt + 3, UpperBound);
  }

  template <typename Var1, typename Var2>
  static bool get_satisfying_assignment(const Var1 &var1, const Var2 &var2,
                                        pair<DomainInt, DomainInt> &assign) {
    DomainInt min = var1.getMin();
    if (var2.getMin() > min)
      min = var2.getMin();
    DomainInt max = var1.getMax();
    if (var2.getMax() < max)
      max = var2.getMax();
    for (DomainInt i = min; i <= max; i++) {
      if (var1.inDomain(i) && var2.inDomain(i)) {
        assign = make_pair(i, i);
        return true;
      }
    }
    return false;
  }

  template <typename Var1, typename Var2>
  static AbstractConstraint *reverse_constraint(const Var1 &var1, const Var2 &var2) {
    NeqConstraintBinary<Var1, Var2> *t = new NeqConstraintBinary<Var1, Var2>(var1, var2);
    return (AbstractConstraint *)t;
  }

  typedef NeqIterated reverse_operator;
};

struct NeqIterated {
  static string constraint_name() { return "hamming"; }

  static SysInt dynamic_trigger_count() { return 2; }

  static bool check_assignment(DomainInt i, DomainInt j) { return i != j; }

  template <typename VarType1, typename VarType2>
  static bool no_support_for_pair(VarType1 &var1, VarType2 &var2) {
    return var1.isAssigned() && var2.isAssigned() &&
           var1.getAssignedValue() == var2.getAssignedValue();
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var1(VarType1 &var1, VarType2 &var2) {
    if (var1.isAssigned())
      remove_value(var1.getAssignedValue(), var2);
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var2(VarType1 &var1, VarType2 &var2) {
    if (var2.isAssigned())
      remove_value(var2.getAssignedValue(), var1);
  }

  template <typename VarType1, typename VarType2>
  static void add_triggers(AbstractConstraint *ac, VarType1 &var1, VarType2 &var2, DomainInt dt) {
    ac->moveTriggerInt(var1, dt, Assigned);
    ac->moveTriggerInt(var2, dt + 1, Assigned);
  }

  template <typename Var>
  static void remove_value(DomainInt val, Var &var) {
    if (var.isBound()) {
      if (var.getMin() == val)
        var.setMin(val + 1);
      else if (var.getMax() == val)
        var.setMax(val - 1);
    } else {
      var.removeFromDomain(val);
    }
  }

  template <typename Var1, typename Var2>
  static bool get_satisfying_assignment(const Var1 &var1, const Var2 &var2,
                                        pair<DomainInt, DomainInt> &assign) {
    if (var1.isAssigned() && var2.isAssigned() &&
        var1.getAssignedValue() == var2.getAssignedValue())
      return false;

    if (var1.isAssigned()) {
      if (var2.getMin() != var1.getAssignedValue())
        assign = make_pair(var1.getAssignedValue(), var2.getMin());
      else
        assign = make_pair(var1.getAssignedValue(), var2.getMax());
    } else {
      if (var1.getMin() != var2.getMin())
        assign = make_pair(var1.getMin(), var2.getMin());
      else
        assign = make_pair(var1.getMax(), var2.getMin());
    }
    return true;
  }

  template <typename Var1, typename Var2>
  static AbstractConstraint *reverse_constraint(const Var1 &var1, const Var2 &var2) {
    EqualConstraint<Var1, Var2> *t = new EqualConstraint<Var1, Var2>(var1, var2);
    return (AbstractConstraint *)t;
  }

  typedef EqIterated reverse_operator;
};

struct LessIterated {
  static bool check_assignment(DomainInt i, DomainInt j) { return i < j; }

  static SysInt dynamic_trigger_count() { return 2; }

  template <typename VarType1, typename VarType2>
  static bool no_support_for_pair(VarType1 &var1, VarType2 &var2) {
    return var1.getMin() >= var2.getMax();
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var1(VarType1 &var1, VarType2 &var2) {
    var2.setMin(var1.getMin() + 1);
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var2(VarType1 &var1, VarType2 &var2) {
    var1.setMax(var2.getMax() - 1);
  }

  template <typename VarType1, typename VarType2>
  static void add_triggers(AbstractConstraint *ac, VarType1 &var1, VarType2 &var2, DomainInt dt) {
    ac->moveTriggerInt(var1, dt, LowerBound);
    ac->moveTriggerInt(var2, dt + 1, UpperBound);
  }

  template <typename Var1, typename Var2>
  static bool get_satisfying_assignment(const Var1 &var1, const Var2 &var2,
                                        pair<DomainInt, DomainInt> &assign) {
    if (var1.getMin() < var2.getMax()) {
      assign = make_pair(var1.getMin(), var2.getMax());
      return true;
    } else
      return false;
  }

  template <typename Var1, typename Var2>
  static AbstractConstraint *reverse_constraint(const Var1 &var1, const Var2 &var2) {
    LeqConstraint<Var2, Var1, compiletime_val<SysInt, 0>> *t =
        new LeqConstraint<Var2, Var1, compiletime_val<SysInt, 0>>(var2, var1,
                                                                  compiletime_val<SysInt, 0>());
    return (AbstractConstraint *)t;
  }
};

struct BothNonZeroIterated {
  static bool check_assignment(DomainInt i, DomainInt j) { return i > 0 && j > 0; }

  static SysInt dynamic_trigger_count() { return 2; }

  template <typename VarType1, typename VarType2>
  static bool no_support_for_pair(VarType1 &var1, VarType2 &var2) {
    return var1.getMax() <= 0 || var2.getMax() <= 0;
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var1(VarType1 &var1, VarType2 &var2) {
    var2.setMin(1);
  }

  template <typename VarType1, typename VarType2>
  static void propagate_from_var2(VarType1 &var1, VarType2 &var2) {
    var1.setMin(1);
  }

  template <typename VarType1, typename VarType2>
  static void add_triggers(AbstractConstraint *ac, VarType1 &var1, VarType2 &var2, DomainInt dt) {
    ac->moveTriggerInt(var1, dt, UpperBound);
    ac->moveTriggerInt(var2, dt + 1, UpperBound);
  }

  template <typename Var1, typename Var2>
  static bool get_satisfying_assignment(const Var1 &var1, const Var2 &var2,
                                        pair<DomainInt, DomainInt> &assign) {
    if (var1.getMax() > 0 && var2.getMax() > 0) {
      assign = make_pair(var1.getMax(), var2.getMax());
      return true;
    } else
      return false;
  }

  template <typename Var1, typename Var2>
  static AbstractConstraint *reverse_constraint(const Var1 &var1, const Var2 &var2) {
    ProductConstraint<Var1, Var2, ConstantVar> *t =
        new ProductConstraint<Var1, Var2, ConstantVar>(var1, var2, ConstantVar(0));
    return (AbstractConstraint *)t;
  }
};

/** Constraints two vectors of variables to be not equal.
  *
  *  \ingroup Constraints
*/
template <typename VarArray1, typename VarArray2, typename Operator = NeqIterated>
struct VecNeqDynamic : public AbstractConstraint {
  virtual string constraint_name() { return "watchvecneq"; }

  CONSTRAINT_ARG_LIST2(var_array1, var_array2);

  typedef typename VarArray1::value_type VarRef1;
  typedef typename VarArray2::value_type VarRef2;

  VarArray1 var_array1;
  VarArray2 var_array2;

  SysInt watched_index0;
  SysInt watched_index1;

  Reversible<bool> propagate_mode;
  SysInt index_to_propagate;

  VecNeqDynamic(const VarArray1 &_array1, const VarArray2 &_array2)
      : var_array1(_array1), var_array2(_array2), propagate_mode(false) {
    D_ASSERT(var_array1.size() == var_array2.size());
  }

  virtual SysInt dynamic_trigger_count() { return Operator::dynamic_trigger_count() * 2; }

  bool no_support_for_index(SysInt index) {
    return Operator::no_support_for_pair(var_array1[index], var_array2[index]);
  }

  void add_triggers(SysInt index, DomainInt dt) {
    Operator::add_triggers(this, var_array1[index], var_array2[index], dt);
  }

  virtual void full_propagate() {
    P("VecNeq full prop");
    SysInt size = var_array1.size();
    SysInt index = 0;

    // Find first pair we could watch.
    while (index < size && no_support_for_index(index))
      ++index;

    // Vectors are assigned and equal.
    if (index == size) {
      getState().setFailed(true);
      return;
    }

    watched_index0 = index;

    ++index;

    // Now, is there another fine pair?
    while (index < size && no_support_for_index(index))
      ++index;

    // There is only one possible pair allowed...
    if (index == size) {
      propagate_from_var1(watched_index0);
      propagate_from_var2(watched_index0);
      propagate_mode = true;
      index_to_propagate = watched_index0;
      add_triggers(watched_index0, 0);
      return;
    }

    watched_index1 = index;

    add_triggers(watched_index0, 0);
    add_triggers(watched_index1, 2);
  }

  void propagate_from_var1(SysInt index) {
    Operator::propagate_from_var1(var_array1[index], var_array2[index]);
  }

  void propagate_from_var2(SysInt index) {
    Operator::propagate_from_var2(var_array1[index], var_array2[index]);
  }

  virtual void propagateDynInt(SysInt trigger_activated) {
    PROP_INFO_ADDONE(DynVecNeq);
    P("VecNeq prop");

    if (propagate_mode) {
      P("Propagating: " << index_to_propagate);
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

    if (triggerpair == 0) {
      original_index = watched_index0;
      other_index = watched_index1;
    } else {
      original_index = watched_index1;
      other_index = watched_index0;
    }

    if (propagate_mode) {
      // If this is true, the other index got assigned.
      if (index_to_propagate != original_index)
        return;

      if (triggerarray == 1) {
        propagate_from_var1(index_to_propagate);
      } else {
        propagate_from_var2(index_to_propagate);
      }
      return;
    }

    // Check if propagation has caused a loss of support.
    if (!no_support_for_index(original_index))
      return;

    SysInt index = original_index + 1;

    SysInt size = var_array1.size();

    while ((index < size && no_support_for_index(index)) || index == other_index)
      ++index;

    if (index == size) {
      index = 0;
      while ((index < original_index && no_support_for_index(index)) || index == other_index)
        ++index;

      if (index == original_index) {
        // This is the only possible non-equal index.
        P("Entering propagate mode for index " << index_to_propagate);
        propagate_mode = true;
        index_to_propagate = other_index;
        propagate_from_var1(other_index);
        propagate_from_var2(other_index);
        return;
      }
    }

    if (triggerpair == 0)
      watched_index0 = index;
    else
      watched_index1 = index;

    D_ASSERT(watched_index0 != watched_index1);
    add_triggers(index, triggerpair * 2);
  }

  virtual BOOL check_assignment(DomainInt *v, SysInt v_size) {
    SysInt v_size1 = var_array1.size();
    for (SysInt i = 0; i < v_size1; ++i)
      if (Operator::check_assignment(v[i], v[i + v_size1]))
        return true;
    return false;
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> vars;
    vars.reserve(var_array1.size() + var_array2.size());
    for (UnsignedSysInt i = 0; i < var_array1.size(); ++i)
      vars.push_back(AnyVarRef(var_array1[i]));
    for (UnsignedSysInt i = 0; i < var_array2.size(); ++i)
      vars.push_back(AnyVarRef(var_array2[i]));
    return vars;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>> &assignment) {
    pair<DomainInt, DomainInt> assign;
    for (SysInt i = 0; i < (SysInt)var_array1.size(); ++i) {
      if (Operator::get_satisfying_assignment(var_array1[i], var_array2[i], assign)) {
        D_ASSERT(var_array1[i].inDomain(assign.first));
        D_ASSERT(var_array2[i].inDomain(assign.second));
        D_ASSERT(Operator::check_assignment(assign.first, assign.second));
        assignment.push_back(make_pair(i, assign.first));
        assignment.push_back(make_pair(i + var_array1.size(), assign.second));
        return true;
      }
    }
    return false;
  }

  virtual AbstractConstraint *reverse_constraint() {
    vector<AbstractConstraint *> con;
    for (SysInt i = 0; i < (SysInt)var_array1.size(); i++) {
      con.push_back(Operator::reverse_constraint(var_array1[i], var_array2[i]));
    }
    return new Dynamic_AND(con);
    /*vector<AnyVarRef> t;
    for(SysInt i=0; i<var_array1.size(); i++) t.push_back(var_array1[i]);
    for(SysInt i=0; i<var_array2.size(); i++) t.push_back(var_array2[i]);
    return forward_check_negation(this);*/
  }
};

template <typename VarArray1, typename VarArray2>
AbstractConstraint *BuildCT_WATCHED_VECNEQ(const VarArray1 &varray1, const VarArray2 &varray2,
                                           ConstraintBlob &) {
  return new VecNeqDynamic<VarArray1, VarArray2>(varray1, varray2);
}

// these two don't seem to be used anywhere
template <typename VarArray1, typename VarArray2>
AbstractConstraint *BuildCT_WATCHED_VEC_OR_LESS(const VarArray1 &varray1, const VarArray2 &varray2,
                                                ConstraintBlob &) {
  return new VecNeqDynamic<VarArray1, VarArray2, LessIterated>(varray1, varray2);
}

template <typename VarArray1, typename VarArray2>
AbstractConstraint *BuildCT_WATCHED_VEC_OR_AND(const VarArray1 &varray1, const VarArray2 &varray2,
                                               ConstraintBlob &) {
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
