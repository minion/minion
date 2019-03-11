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
  virtual string constraintName() {
    return Operator::constraintName();
  }

  CONSTRAINT_ARG_LIST3(var_array1, var_array2, original_distance());

  // 10 - 3 + 1 = 8
  // 10 - 8 + 1 = 3
  DomainInt original_distance() {
    if(constraintName() == "hamming")
      return hamming_distance;
    if(constraintName() == "not-hamming")
      return (SysInt)var_array1.size() - hamming_distance + 1;
    abort();
  }
  typedef typename VarArray1::value_type VarRef1;
  typedef typename VarArray2::value_type VarRef2;

  VarArray1 var_array1;
  VarArray2 var_array2;
  DomainInt num_to_watch;
  DomainInt hamming_distance;
  vector<DomainInt> watchedValues;
  vector<DomainInt> unwatchedValues;

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

  virtual SysInt dynamicTriggerCount() {
    return checked_cast<SysInt>(Operator::dynamicTriggerCount() * num_to_watch);
  }

  bool no_support_for_index(DomainInt index_in) {
    const SysInt index = checked_cast<SysInt>(index_in);
    return Operator::no_support_for_pair(var_array1[index], var_array2[index]);
  }

  void add_triggers(DomainInt index_in, DomainInt dt) {
    const SysInt index = checked_cast<SysInt>(index_in);
    Operator::add_triggers(this, var_array1[index], var_array2[index], dt);
  }

  virtual void fullPropagate() {

    // Check if the constraint is trivial, if so just exit now.
    if(num_to_watch <= 1)
      return;

    watchedValues.resize(checked_cast<SysInt>(num_to_watch));

    SysInt size = var_array1.size();
    SysInt index = 0;
    SysInt found_matches = 0;
    // Find first pair we could watch.

    while(found_matches < num_to_watch && index < size) {
      while(index < size && no_support_for_index(index)) {
        ++index;
      }
      if(index != size) {
        watchedValues[found_matches] = index;
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
        propagate_from_var1(watchedValues[i]);
        propagate_from_var2(watchedValues[i]);
        add_triggers(watchedValues[i], Operator::dynamicTriggerCount() * i);
      }
      return;
    }

    // Found enough values to watch, no propagation yet!
    for(SysInt i = 0; i < num_to_watch; ++i) {
      add_triggers(watchedValues[i], Operator::dynamicTriggerCount() * i);
    }

    // Setup the 'unwatched values' array.
    initalise_unwatchedValues();
  }

  void initalise_unwatchedValues() {
    unwatchedValues.resize(0);
    for(SysInt i = 0; i < (SysInt)var_array1.size(); ++i) {
      bool found = false;
      for(SysInt j = 0; j < (SysInt)watchedValues.size(); ++j) {
        if(i == watchedValues[j])
          found = true;
      }
      if(!found)
        unwatchedValues.push_back(i);
    }

    random_shuffle(unwatchedValues.begin(), unwatchedValues.end());
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
    SysInt triggerpair = trigger_activated / Operator::dynamicTriggerCount();
    D_ASSERT(triggerpair >= 0 && triggerpair < num_to_watch);

    /*printf("propmode=%d, triggerpair=%d, trigger_activated=%d\n",
      (SysInt)propagate_mode, (SysInt)triggerpair, (SysInt)trigger_activated);

    for(SysInt i = 0; i < (SysInt)watchedValues.size(); ++i)
      printf("%d,", watchedValues[i]);

    printf(":");
    for(SysInt i = 0; i < (SysInt)unwatchedValues.size(); ++i)
      printf("%d,", unwatchedValues[i]);
    printf("\n");

    for(SysInt i = 0; i < var_array1.size(); ++i)
      cout << var_array1[i].min() << ":" << var_array1[i].max() << ",";

    cout << endl;

    for(SysInt i = 0; i < var_array2.size(); ++i)
      cout << var_array2[i].min() << ":" << var_array2[i].max() << ",";

    cout << endl;*/

    if(propagate_mode) {
      if(index_to_not_propagate == watchedValues[triggerpair])
        return;

      // assumes that the first set of Operator::dynamicTriggerCount()/2
      // triggers are on var1, and the other set are on var2.
      if(trigger_activated % Operator::dynamicTriggerCount() <
         Operator::dynamicTriggerCount() / 2) {
        propagate_from_var1(watchedValues[triggerpair]);
      } else {
        propagate_from_var2(watchedValues[triggerpair]);
      }
      return;
    }

    // Check if propagation has caused a loss of support.
    if(!no_support_for_index(watchedValues[triggerpair]))
      return;

    SysInt index = 0;
    SysInt unwatchedSize = unwatchedValues.size();
    while(index < unwatchedSize && no_support_for_index(unwatchedValues[index]))
      index++;

    if(index == unwatchedSize) {
      // This is the only possible non-equal index.
      propagate_mode = true;
      index_to_not_propagate = watchedValues[triggerpair];

      //     printf("!propmode=%d, triggerpair=%d, trigger_activated=%d,
      //     nopropindex=%d\n",
      //        (SysInt)propagate_mode, (SysInt)triggerpair,
      //        (SysInt)trigger_activated, (SysInt)index_to_not_propagate);

      for(SysInt i = 0; i < (SysInt)watchedValues.size(); ++i) {
        if(i != triggerpair) {
          propagate_from_var1(watchedValues[i]);
          propagate_from_var2(watchedValues[i]);
        }
      }
      return;
    }

    swap(watchedValues[triggerpair], unwatchedValues[index]);

    add_triggers(watchedValues[triggerpair], triggerpair * Operator::dynamicTriggerCount());
  }

  virtual BOOL checkAssignment(DomainInt* v, SysInt vSize) {
    UnsignedSysInt vSize1 = var_array1.size();
    SysInt count = 0;
    for(UnsignedSysInt i = 0; i < vSize1; ++i)
      if(Operator::checkAssignment(v[i], v[i + vSize1]))
        count++;
    return (count >= hamming_distance);
  }

  virtual vector<AnyVarRef> getVars() {
    vector<AnyVarRef> vars;
    vars.reserve(var_array1.size() + var_array2.size());
    for(UnsignedSysInt i = 0; i < var_array1.size(); ++i)
      vars.push_back(AnyVarRef(var_array1[i]));
    for(UnsignedSysInt i = 0; i < var_array2.size(); ++i)
      vars.push_back(AnyVarRef(var_array2[i]));
    return vars;
  }

  virtual bool getSatisfyingAssignment(box<pair<SysInt, DomainInt>>& assignment) {
    if(num_to_watch <= 1)
      return true;

    pair<DomainInt, DomainInt> assign;
    SysInt found_satisfying = 0;
    for(SysInt i = 0; i < (SysInt)var_array1.size(); ++i) {
      if(Operator::getSatisfyingAssignment(var_array1[i], var_array2[i], assign)) {
        found_satisfying++;
        D_ASSERT(var_array1[i].inDomain(assign.first));
        D_ASSERT(var_array2[i].inDomain(assign.second));
        D_ASSERT(Operator::checkAssignment(assign.first, assign.second));
        assignment.push_back(make_pair(i, assign.first));
        assignment.push_back(make_pair(i + var_array1.size(), assign.second));
        if(found_satisfying == hamming_distance)
          return true;
      }
    }
    // If we didn't get enough,
    return false;
  }

  virtual AbstractConstraint* reverseConstraint() {
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
