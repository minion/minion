/* Minion Constraint Solver
http://minion.sourceforge.net

For Licence Information see file LICENSE.txt

  $Id: dynamic_vecneq.h 1117 2008-02-15 17:19:14Z caj $
*/

/* Minion
  * Copyright (C) 2006
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

/** @help constraints;hamming Description
The constraint

   hamming(X,Y,c)

ensures that the hamming distance between X and Y is at least c. That is, that
the size of the set {i | X[i] != y[i]} is greater than or equal to c.
*/

#ifndef CONSTRAINT_DYNAMIC_SUM_OR_H
#define CONSTRAINT_DYNAMIC_SUM_OR_H

// For operators
#include "dynamic_vecneq.h"

template <typename VarArray1, typename VarArray2, typename Operator = NeqIterated>
struct VecCountDynamic : public AbstractConstraint {
  virtual string constraint_name() {
    return Operator::constraint_name();
  }

  CONSTRAINT_ARG_LIST3(var_array1, var_array2, original_distance());

  // 10 - 3 + 1 = 8
  // 10 - 8 + 1 = 3
  DomainInt original_distance() {
    if(constraint_name() == "hamming")
      return hamming_distance;
    if(constraint_name() == "not-hamming")
      return (SysInt)var_array1.size() - hamming_distance + 1;
    abort();
  }
  typedef typename VarArray1::value_type VarRef1;
  typedef typename VarArray2::value_type VarRef2;

  VarArray1 var_array1;
  VarArray2 var_array2;
  DomainInt num_to_watch;
  DomainInt hamming_distance;
  vector<DomainInt> watched_values;
  vector<DomainInt> unwatched_values;

  Reversible<bool> propagate_mode;
  DomainInt index_to_not_propagate;

  VecCountDynamic(const VarArray1& _array1, const VarArray2& _array2, DomainInt _hamming_distance)
      : var_array1(_array1),
        var_array2(_array2),
        num_to_watch(_hamming_distance + 1),
        hamming_distance(_hamming_distance),
        propagate_mode(false),
        index_to_not_propagate(-1) {
    if(num_to_watch <= 1)
      num_to_watch = 0;
    D_ASSERT(var_array1.size() == var_array2.size());
  }

  virtual SysInt dynamic_trigger_count() {
    return checked_cast<SysInt>(Operator::dynamic_trigger_count() * num_to_watch);
  }

  bool no_support_for_index(DomainInt index_in) {
    const SysInt index = checked_cast<SysInt>(index_in);
    return Operator::no_support_for_pair(var_array1[index], var_array2[index]);
  }

  void add_triggers(DomainInt index_in, DomainInt dt) {
    const SysInt index = checked_cast<SysInt>(index_in);
    Operator::add_triggers(this, var_array1[index], var_array2[index], dt);
  }

  virtual void full_propagate() {

    // Check if the constraint is trivial, if so just exit now.
    if(num_to_watch <= 1)
      return;

    watched_values.resize(checked_cast<SysInt>(num_to_watch));

    SysInt size = var_array1.size();
    SysInt index = 0;
    SysInt found_matches = 0;
    // Find first pair we could watch.

    while(found_matches < num_to_watch && index < size) {
      while(index < size && no_support_for_index(index)) {
        ++index;
      }
      if(index != size) {
        watched_values[found_matches] = index;
        ++found_matches;
        ++index;
      }
    }

    // Failed to find enough watches
    if(found_matches < num_to_watch - 1) {
      getState().setFailed(true);
      return;
    }

    // Found exactly as many values as we need to propagate
    if(found_matches == num_to_watch - 1) {
      index_to_not_propagate = -1;
      propagate_mode = true;
      for(SysInt i = 0; i < num_to_watch - 1; ++i) {
        propagate_from_var1(watched_values[i]);
        propagate_from_var2(watched_values[i]);
        add_triggers(watched_values[i], Operator::dynamic_trigger_count() * i);
      }
      return;
    }

    // Found enough values to watch, no propagation yet!
    for(SysInt i = 0; i < num_to_watch; ++i) {
      add_triggers(watched_values[i], Operator::dynamic_trigger_count() * i);
    }

    // Setup the 'unwatched values' array.
    initalise_unwatched_values();
  }

  void initalise_unwatched_values() {
    unwatched_values.resize(0);
    for(SysInt i = 0; i < (SysInt)var_array1.size(); ++i) {
      bool found = false;
      for(SysInt j = 0; j < (SysInt)watched_values.size(); ++j) {
        if(i == watched_values[j])
          found = true;
      }
      if(!found)
        unwatched_values.push_back(i);
    }

    random_shuffle(unwatched_values.begin(), unwatched_values.end());
  }

  void propagate_from_var1(DomainInt index_in) {
    const SysInt index = checked_cast<SysInt>(index_in);
    Operator::propagate_from_var1(var_array1[index], var_array2[index]);
  }

  void propagate_from_var2(DomainInt index_in) {
    const SysInt index = checked_cast<SysInt>(index_in);
    Operator::propagate_from_var2(var_array1[index], var_array2[index]);
  }

  virtual void propagateDynInt(SysInt trigger_activated, DomainDelta) {
    PROP_INFO_ADDONE(DynVecNeq);
    SysInt triggerpair = trigger_activated / Operator::dynamic_trigger_count();
    D_ASSERT(triggerpair >= 0 && triggerpair < num_to_watch);

    /*printf("propmode=%d, triggerpair=%d, trigger_activated=%d\n",
      (SysInt)propagate_mode, (SysInt)triggerpair, (SysInt)trigger_activated);

    for(SysInt i = 0; i < (SysInt)watched_values.size(); ++i)
      printf("%d,", watched_values[i]);

    printf(":");
    for(SysInt i = 0; i < (SysInt)unwatched_values.size(); ++i)
      printf("%d,", unwatched_values[i]);
    printf("\n");

    for(SysInt i = 0; i < var_array1.size(); ++i)
      cout << var_array1[i].getMin() << ":" << var_array1[i].getMax() << ",";

    cout << endl;

    for(SysInt i = 0; i < var_array2.size(); ++i)
      cout << var_array2[i].getMin() << ":" << var_array2[i].getMax() << ",";

    cout << endl;*/

    if(propagate_mode) {
      if(index_to_not_propagate == watched_values[triggerpair])
        return;

      // assumes that the first set of Operator::dynamic_trigger_count()/2
      // triggers are on var1, and the other set are on var2.
      if(trigger_activated % Operator::dynamic_trigger_count() <
         Operator::dynamic_trigger_count() / 2) {
        propagate_from_var1(watched_values[triggerpair]);
      } else {
        propagate_from_var2(watched_values[triggerpair]);
      }
      return;
    }

    // Check if propagation has caused a loss of support.
    if(!no_support_for_index(watched_values[triggerpair]))
      return;

    SysInt index = 0;
    SysInt unwatched_size = unwatched_values.size();
    while(index < unwatched_size && no_support_for_index(unwatched_values[index]))
      index++;

    if(index == unwatched_size) {
      // This is the only possible non-equal index.
      propagate_mode = true;
      index_to_not_propagate = watched_values[triggerpair];

      //     printf("!propmode=%d, triggerpair=%d, trigger_activated=%d,
      //     nopropindex=%d\n",
      //        (SysInt)propagate_mode, (SysInt)triggerpair,
      //        (SysInt)trigger_activated, (SysInt)index_to_not_propagate);

      for(SysInt i = 0; i < (SysInt)watched_values.size(); ++i) {
        if(i != triggerpair) {
          propagate_from_var1(watched_values[i]);
          propagate_from_var2(watched_values[i]);
        }
      }
      return;
    }

    swap(watched_values[triggerpair], unwatched_values[index]);

    add_triggers(watched_values[triggerpair], triggerpair * Operator::dynamic_trigger_count());
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size) {
    UnsignedSysInt v_size1 = var_array1.size();
    SysInt count = 0;
    for(UnsignedSysInt i = 0; i < v_size1; ++i)
      if(Operator::check_assignment(v[i], v[i + v_size1]))
        count++;
    return (count >= hamming_distance);
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

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>>& assignment) {
    if(num_to_watch <= 1)
      return true;

    pair<DomainInt, DomainInt> assign;
    SysInt found_satisfying = 0;
    for(SysInt i = 0; i < (SysInt)var_array1.size(); ++i) {
      if(Operator::get_satisfying_assignment(var_array1[i], var_array2[i], assign)) {
        found_satisfying++;
        D_ASSERT(var_array1[i].inDomain(assign.first));
        D_ASSERT(var_array2[i].inDomain(assign.second));
        D_ASSERT(Operator::check_assignment(assign.first, assign.second));
        assignment.push_back(make_pair(i, assign.first));
        assignment.push_back(make_pair(i + var_array1.size(), assign.second));
        if(found_satisfying == hamming_distance)
          return true;
      }
    }
    // If we didn't get enough,
    return false;
  }

  virtual AbstractConstraint* reverse_constraint() {
    return new VecCountDynamic<VarArray1, VarArray2, typename Operator::reverse_operator>(
        var_array1, var_array2, (SysInt)var_array1.size() - hamming_distance + 1);
  }
};

template <typename VarArray1, typename VarArray2>
AbstractConstraint* VecOrCountConDynamic(const VarArray1& varray1, const VarArray2& varray2,
                                         DomainInt i) {
  return new VecCountDynamic<VarArray1, VarArray2>(varray1, varray2, i);
}

template <typename VarArray1, typename VarArray2>
AbstractConstraint* NotVecOrCountConDynamic(const VarArray1& varray1, const VarArray2& varray2,
                                            DomainInt i) {
  return new VecCountDynamic<VarArray1, VarArray2, EqIterated>(varray1, varray2,
                                                               (SysInt)varray1.size() - i + 1);
}

template <typename T1, typename T2>
AbstractConstraint* BuildCT_WATCHED_HAMMING(const T1& t1, const T2& t2, ConstraintBlob& b) {
  return VecOrCountConDynamic(t1, t2, b.constants[0][0]);
}

/* JSON
{ "type": "constraint",
  "name": "hamming",
  "internal_name": "CT_WATCHED_HAMMING",
  "args": [ "read_list", "read_list", "read_constant" ]
}
*/

template <typename T1, typename T2>
AbstractConstraint* BuildCT_WATCHED_NOT_HAMMING(const T1& t1, const T2& t2, ConstraintBlob& b) {
  return NotVecOrCountConDynamic(t1, t2, b.constants[0][0]);
}

/* JSON
{ "type": "constraint",
  "name": "not-hamming",
  "internal_name": "CT_WATCHED_NOT_HAMMING",
  "args": [ "read_list", "read_list", "read_constant" ]
}
*/
#endif
