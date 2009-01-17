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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

template<typename VarArray,  typename VarSum>
AbstractConstraint*
BuildCT_LEQSUM(StateObj* stateObj, const VarArray& _var_array, const light_vector<VarSum>& _var_sum, ConstraintBlob&)
{ 
  if(_var_array.size() == 2)
  {
    array<typename VarArray::value_type, 2> v_array;
    for(int i = 0; i < 2; ++i)
      v_array[i] = _var_array[i];
    return LightLessEqualSumCon(stateObj, v_array, _var_sum[0]);
  }
  else
  {
    return new LessEqualSumConstraint<VarArray, VarSum>(stateObj, _var_array, _var_sum[0]); 
  }
}

inline AbstractConstraint*
BuildCT_LEQSUM(StateObj* stateObj, const light_vector<BoolVarRef>& var_array, const light_vector<ConstantVar>& var_sum, ConstraintBlob&)
{ 
  runtime_val t2(checked_cast<int>(var_sum[0].getAssignedValue()));
  return BoolLessEqualSumCon(stateObj, var_array, t2);
}

template<typename VarArray,  typename VarSum>
AbstractConstraint*
BuildCT_GEQSUM(StateObj* stateObj, const light_vector<VarArray>& _var_array, const light_vector<VarSum>& _var_sum, ConstraintBlob&)
{ 
  if(_var_array.size() == 2)
  {
    array<VarArray, 2> v_array;
    for(int i = 0; i < 2; ++i)
      v_array[i] = _var_array[i];
    return LightGreaterEqualSumCon(stateObj, v_array, _var_sum[0]);
  }
  else
  {
    return (new LessEqualSumConstraint<light_vector<typename NegType<VarArray>::type>, 
      typename NegType<VarSum>::type>(stateObj, VarNegRef(_var_array), VarNegRef(_var_sum[0]))); 
  }
}

inline AbstractConstraint*
BuildCT_GEQSUM(StateObj* stateObj, const light_vector<BoolVarRef>& var_array, const light_vector<ConstantVar>& var_sum, ConstraintBlob&)
{ 
  runtime_val t2(checked_cast<int>(var_sum[0].getAssignedValue()));
  return BoolGreaterEqualSumCon(stateObj, var_array, t2);
}

#endif
