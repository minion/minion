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

// This files branches between the 3 implementations of sum:

// constraint_fullsum.h  : Standard implementation.
// constraint_lightsum.h : Only for very short arrays by not storing any state.
// constraint_sum_bool.h : Only for arrays of booleans, summed to a constant.

#ifndef CONSTRAINT_SUM_QPWO
#define CONSTRAINT_SUM_QPWO

#include "constraint_fullsum.h"
#include "constraint_lightsum.h"
#include "constraint_sum_bool.h"

template <typename VarArray, typename VarSum>
AbstractConstraint* greaterEqualSumConstraint(const vector<VarArray>& _var_array,
                                              const VarSum _var_sum) {
  if(_var_array.size() == 2) {
    std::array<VarArray, 2> v_array;
    for(SysInt i = 0; i < 2; ++i)
      v_array[i] = _var_array[i];
    return LightGreaterEqualSumCon(v_array, _var_sum);
  } else {
    return (new LessEqualSumConstraint<vector<typename NegType<VarArray>::type>,
                                       typename NegType<VarSum>::type>(VarNegRef(_var_array),
                                                                       VarNegRef(_var_sum)));
  }
}

template <typename VarArray, typename VarSum>
AbstractConstraint* lessEqualSumConstraint(const vector<VarArray>& _var_array,
                                           const VarSum _var_sum) {
  if(_var_array.size() == 2) {
    std::array<VarArray, 2> v_array;
    for(SysInt i = 0; i < 2; ++i)
      v_array[i] = _var_array[i];
    return LightLessEqualSumCon(v_array, _var_sum);
  } else {
    return new LessEqualSumConstraint<vector<VarArray>, VarSum>(_var_array, _var_sum);
  }
}

template <typename VarArray, typename VarSum>
AbstractConstraint* BuildCT_LEQSUM(const VarArray& _var_array, const vector<VarSum>& _var_sum,
                                   ConstraintBlob&) {
  if(_var_array.size() == 2) {
    std::array<typename VarArray::value_type, 2> v_array;
    for(SysInt i = 0; i < 2; ++i)
      v_array[i] = _var_array[i];
    return LightLessEqualSumCon(v_array, _var_sum[0]);
  } else {
    return new LessEqualSumConstraint<VarArray, VarSum>(_var_array, _var_sum[0]);
  }
}

inline AbstractConstraint* BuildCT_LEQSUM(const vector<BoolVarRef>& var_array,
                                          const vector<ConstantVar>& var_sum, ConstraintBlob&) {
  SysInt t2(checked_cast<SysInt>(var_sum[0].assignedValue()));
  return BoolLessEqualSumCon(var_array, t2);
}

/* JSON
{ "type": "constraint",
  "name": "sumleq",
  "internal_name": "CT_LEQSUM",
  "args": [ "read_list", "read_var" ]
}
*/

template <typename VarArray, typename VarSum>
AbstractConstraint* BuildCT_GEQSUM(const vector<VarArray>& _var_array,
                                   const vector<VarSum>& _var_sum, ConstraintBlob&) {
  if(_var_array.size() == 2) {
    std::array<VarArray, 2> v_array;
    for(SysInt i = 0; i < 2; ++i)
      v_array[i] = _var_array[i];
    return LightGreaterEqualSumCon(v_array, _var_sum[0]);
  } else {
    return (new LessEqualSumConstraint<vector<typename NegType<VarArray>::type>,
                                       typename NegType<VarSum>::type>(VarNegRef(_var_array),
                                                                       VarNegRef(_var_sum[0])));
  }
}

inline AbstractConstraint* BuildCT_GEQSUM(const vector<BoolVarRef>& var_array,
                                          const vector<ConstantVar>& var_sum, ConstraintBlob&) {
  SysInt t2(checked_cast<SysInt>(var_sum[0].assignedValue()));
  return BoolGreaterEqualSumCon(var_array, t2);
}

/* JSON
{ "type": "constraint",
  "name": "sumgeq",
  "internal_name": "CT_GEQSUM",
  "args": [ "read_list", "read_var" ]
}
*/

// Don't pass in the vectors by reference, as we might need to copy them.
template <typename T1, typename T2>
AbstractConstraint* BuildCT_WEIGHTGEQSUM(vector<T1> vec, const vector<T2>& t2, ConstraintBlob& b) {
  vector<DomainInt> scale = b.constants[0];
  // Preprocess to remove any multiplications by 0, both for efficency
  // and correctness
  if(scale.size() != vec.size()) {
    FAIL_EXIT("In a weighted sum constraint, the vector of weights must have "
              "the same length as the vector of variables.");
  }
  for(UnsignedSysInt i = 0; i < scale.size(); ++i) {
    if(scale[i] == 0) {
      scale.erase(scale.begin() + i);
      vec.erase(vec.begin() + i);
      --i; // So we don't miss an element.
    }
  }

  BOOL multipliers_size_one = true;
  for(UnsignedSysInt i = 0; i < scale.size(); ++i) {
    if(scale[i] != 1 && scale[i] != -1) {
      multipliers_size_one = false;
      i = scale.size();
    }
  }

  if(multipliers_size_one) {
    vector<SwitchNeg<T1>> mult_vars(vec.size());
    for(UnsignedSysInt i = 0; i < vec.size(); ++i)
      mult_vars[i] = SwitchNeg<T1>(vec[i], scale[i]);
    return BuildCT_GEQSUM(mult_vars, t2, b);
  } else {
    vector<MultiplyVar<T1>> mult_vars(vec.size());
    for(UnsignedSysInt i = 0; i < vec.size(); ++i)
      mult_vars[i] = MultiplyVar<T1>(vec[i], scale[i]);
    return BuildCT_GEQSUM(mult_vars, t2, b);
  }
}

/* JSON
{ "type": "constraint",
  "name": "weightedsumgeq",
  "internal_name": "CT_WEIGHTGEQSUM",
  "args": [ "read_constant_list", "read_list", "read_var" ]
}
*/

template <typename T1, typename T2>
AbstractConstraint* BuildCT_WEIGHTLEQSUM(vector<T1> vec, const vector<T2>& t2, ConstraintBlob& b) {
  vector<DomainInt> scale = b.constants[0];
  // Preprocess to remove any multiplications by 0, both for efficency
  // and correctness
  if(scale.size() != vec.size()) {
    FAIL_EXIT("In a weighted sum constraint, the vector of weights must have "
              "the same length to the vector of variables.");
  }
  for(UnsignedSysInt i = 0; i < scale.size(); ++i) {
    if(scale[i] == 0) {
      scale.erase(scale.begin() + i);
      vec.erase(vec.begin() + i);
      --i; // So we don't miss an element.
    }
  }

  BOOL multipliers_size_one = true;
  for(UnsignedSysInt i = 0; i < scale.size(); ++i) {
    if(scale[i] != 1 && scale[i] != -1) {
      multipliers_size_one = false;
      i = scale.size();
    }
  }

  if(multipliers_size_one) {
    vector<SwitchNeg<T1>> mult_vars(vec.size());
    for(UnsignedSysInt i = 0; i < vec.size(); ++i)
      mult_vars[i] = SwitchNeg<T1>(vec[i], scale[i]);
    return BuildCT_LEQSUM(mult_vars, t2, b);
  } else {
    vector<MultiplyVar<T1>> mult_vars(vec.size());
    for(UnsignedSysInt i = 0; i < vec.size(); ++i)
      mult_vars[i] = MultiplyVar<T1>(vec[i], scale[i]);
    return BuildCT_LEQSUM(mult_vars, t2, b);
  }
}

/* JSON
{ "type": "constraint",
  "name": "weightedsumleq",
  "internal_name": "CT_WEIGHTLEQSUM",
  "args": [ "read_constant_list", "read_list", "read_var" ]
}
*/

#endif
