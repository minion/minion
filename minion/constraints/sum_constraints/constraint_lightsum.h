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

#ifndef CONSTRAINT_LIGHTSUM_H_PLOKIJ
#define CONSTRAINT_LIGHTSUM_H_PLOKIJ

// This implementation of sum is designed for very short arrays, by
// not keeping any state.

/// V1 + ... Vn <= X
template <typename VarRef, std::size_t size, typename VarSum, BOOL is_reversed = false>
struct LightLessEqualSumConstraint : public AbstractConstraint {
  virtual string constraint_name() { return "Light<=Sum"; }

  CONSTRAINT_WEIGHTED_REVERSIBLE_ARG_LIST2("weighted", "sumleq", "sumgeq", var_array, var_sum);

  bool no_negatives;

  std::array<VarRef, size> var_array;
  VarSum var_sum;
  LightLessEqualSumConstraint(const std::array<VarRef, size> &_var_array, const VarSum &_var_sum)
      : var_array(_var_array), var_sum(_var_sum) {
    BigInt accumulator = 0;
    for (SysInt i = 0; i < (SysInt)var_array.size(); i++) {
      accumulator += checked_cast<SysInt>(
          max(abs(var_array[i].getInitialMax()), abs(var_array[i].getInitialMin())));
      CHECKSIZE(accumulator, "Sum of bounds of variables too large in sum constraint");
    }
    accumulator +=
        checked_cast<SysInt>(max(abs(var_sum.getInitialMax()), abs(var_sum.getInitialMin())));
    CHECKSIZE(accumulator, "Sum of bounds of variables too large in sum constraint");

    no_negatives = true;
    for (SysInt i = 0; i < (SysInt)var_array.size(); ++i) {
      if (var_array[i].getInitialMin() < 0) {
        no_negatives = false;
        return;
      }
    }
  }

  virtual SysInt dynamic_trigger_count() {
    return var_array.size() + 1;
  }

  void setup_triggers() {
    for (SysInt i = 0; i < (SysInt)var_array.size(); i++) {
      moveTriggerInt(var_array[i], i, LowerBound);
    }
    moveTriggerInt(var_sum, var_array.size(), UpperBound);
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>> &assignment) {
    DomainInt sum_value = 0;
    SysInt v_size = var_array.size();

    if (no_negatives) {
      DomainInt max_sum = var_sum.getMax();
      assignment.push_back(make_pair(v_size, max_sum));
      for (SysInt i = 0; i < v_size && sum_value <= max_sum; ++i) {
        DomainInt min_val = var_array[i].getMin();
        assignment.push_back(make_pair(i, min_val));
        sum_value += min_val;
      }
      return (sum_value <= max_sum);
    } else {
      for (SysInt i = 0; i < v_size; ++i) {
        assignment.push_back(make_pair(i, var_array[i].getMin()));
        sum_value += var_array[i].getMin();
      }
      if (sum_value > var_sum.getMax())
        return false;
      else
        assignment.push_back(make_pair(v_size, var_sum.getMax()));
      return true;
    }
  }

  virtual void propagateDynInt(SysInt prop_val, DomainDelta) {
    PROP_INFO_ADDONE(LightSum);
    DomainInt min_sum = 0;
    for (UnsignedSysInt i = 0; i < size; ++i)
      min_sum += var_array[i].getMin();

    if (prop_val != var_array.size()) {
      var_sum.setMin(min_sum);
    }

    DomainInt slack = var_sum.getMax() - min_sum;
    for (UnsignedSysInt i = 0; i < size; ++i)
      var_array[i].setMax(var_array[i].getMin() + slack);
  }

  virtual void full_propagate() {
    propagateDynInt(var_array.size(), DomainDelta::empty());
    propagateDynInt(0, DomainDelta::empty());
  }

  virtual BOOL check_assignment(DomainInt *v, SysInt v_size) {
    D_ASSERT(v_size == (SysInt)var_array.size() + 1);
    DomainInt sum = 0;
    for (SysInt i = 0; i < v_size - 1; i++)
      sum += v[i];
    return sum <= *(v + v_size - 1);
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> array_copy;
    array_copy.reserve(var_array.size() + 1);
    for (UnsignedSysInt i = 0; i < var_array.size(); ++i)
      array_copy.push_back(var_array[i]);
    array_copy.push_back(var_sum);
    return array_copy;
  }

  virtual AbstractConstraint *reverse_constraint() { return rev_implement<is_reversed>(); }

  template <bool b>
  typename std::enable_if<!b, AbstractConstraint *>::type rev_implement() {
    typedef std::array<typename NegType<VarRef>::type, size> VarArray;
    VarArray new_var_array;
    for (UnsignedSysInt i = 0; i < var_array.size(); ++i)
      new_var_array[i] = VarNegRef(var_array[i]);

    typedef typename ShiftType<typename NegType<VarSum>::type, compiletime_val<SysInt, -1>>::type
        SumType;
    SumType new_sum = ShiftVarRef(VarNegRef(var_sum), compiletime_val<SysInt, -1>());

    return new LightLessEqualSumConstraint<typename NegType<VarRef>::type, size, SumType, true>(
        new_var_array, new_sum);
  }

  template <bool b>
  typename std::enable_if<b, AbstractConstraint *>::type rev_implement() {
    typedef std::array<AnyVarRef, size> VarArray;
    VarArray new_var_array;
    for (UnsignedSysInt i = 0; i < var_array.size(); ++i)
      new_var_array[i] = VarNegRef(var_array[i]);

    typedef typename ShiftType<typename NegType<VarSum>::type, compiletime_val<SysInt, -1>>::type
        SumType;
    SumType new_sum = ShiftVarRef(VarNegRef(var_sum), compiletime_val<SysInt, -1>());

    return new LightLessEqualSumConstraint<AnyVarRef, size, AnyVarRef, true>(new_var_array,
                                                                             new_sum);
  }
};

template <typename VarRef, std::size_t size, typename VarSum>
AbstractConstraint *LightLessEqualSumCon(const std::array<VarRef, size> &_var_array,
                                         const VarSum &_var_sum) {
  return (new LightLessEqualSumConstraint<VarRef, size, VarSum>(_var_array, _var_sum));
}

template <typename VarRef, std::size_t size, typename VarSum>
AbstractConstraint *LightGreaterEqualSumCon(const std::array<VarRef, size> &_var_array,
                                            const VarSum &_var_sum) {
  return (new LightLessEqualSumConstraint<typename NegType<VarRef>::type, size,
                                          typename NegType<VarSum>::type>(VarNegRef(_var_array),
                                                                          VarNegRef(_var_sum)));
}

#endif
