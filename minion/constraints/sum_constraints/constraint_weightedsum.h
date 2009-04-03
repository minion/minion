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

/** @help constraints;weightedsumleq Description
The constraint

   weightedsumleq(constantVec, varVec, total)

ensures that constantVec.varVec <= total, where constantVec.varVec is
the scalar dot product of constantVec and varVec.
*/

/** @help constraints;weightedsumleq References
help constraints weightedsumgeq
help constraints sumleq
help constraints sumgeq
*/

/** @help constraints;weightedsumgeq Description
The constraint

   weightedsumgeq(constantVec, varVec, total)

ensures that constantVec.varVec >= total, where constantVec.varVec is
the scalar dot product of constantVec and varVec.
*/

/** @help constraints;weightedsumgeq References
help constraints weightedsumleq
help constraints sumleq
help constraints sumgeq
*/

#ifndef CONSTRAINT_WEIGHTSUM_H
#define CONSTRAINT_WEIGHTSUM_H

#include "constraint_sum.h"
 
template<typename T1, typename T2>
AbstractConstraint*
BuildCT_WEIGHTLEQSUM(StateObj* stateObj, vector<T1> vec, const vector<T2>& t2, ConstraintBlob& b)
{
  vector<int> scale = b.constants[0];
  // Preprocess to remove any multiplications by 0, both for efficency
  // and correctness
  if(scale.size() != vec.size())
  {
    FAIL_EXIT("In a weighted sum constraint, the vector of weights must have the same length to the vector of variables.");
  }
  for(unsigned i = 0; i < scale.size(); ++i)
  {
    if(scale[i] == 0)
    {
      scale.erase(scale.begin() + i);
      vec.erase(vec.begin() + i);
      --i; // So we don't miss an element.
    }
  }

  BOOL multipliers_size_one = true;
  for(unsigned i = 0; i < scale.size(); ++i)
  {
    if(scale[i] != 1 && scale[i] != -1)
    {
      multipliers_size_one = false;
      i = scale.size();
    }
  }

  if(multipliers_size_one)
  {
    vector<SwitchNeg<T1> > mult_vars(vec.size());
    for(unsigned int i = 0; i < vec.size(); ++i)
      mult_vars[i] = SwitchNeg<T1>(vec[i], scale[i]);
    return BuildCT_LEQSUM(stateObj, mult_vars, t2, b);
  }
  else
  {
    vector<MultiplyVar<T1> > mult_vars(vec.size());
    for(unsigned int i = 0; i < vec.size(); ++i)
      mult_vars[i] = MultiplyVar<T1>(vec[i], scale[i]);
    return BuildCT_LEQSUM(stateObj, mult_vars, t2, b);
  }
}

// Don't pass in the vectors by reference, as we might need to copy them.
template<typename T1, typename T2>
AbstractConstraint*
BuildCT_WEIGHTGEQSUM(StateObj* stateObj, vector<T1> vec, const vector<T2>& t2, ConstraintBlob& b)
{
  vector<int> scale = b.constants[0];
  // Preprocess to remove any multiplications by 0, both for efficency
  // and correctness
  if(scale.size() != vec.size())
  {
    FAIL_EXIT("In a weighted sum constraint, the vector of weights must have the same length as the vector of variables.");
  }
  for(unsigned i = 0; i < scale.size(); ++i)
  {
    if(scale[i] == 0)
    {
      scale.erase(scale.begin() + i);
      vec.erase(vec.begin() + i);
      --i; // So we don't miss an element.
    }
  }

  BOOL multipliers_size_one = true;  
  for(unsigned i = 0; i < scale.size(); ++i)
  {
    if(scale[i] != 1 && scale[i] != -1)
    {
      multipliers_size_one = false;
      i = scale.size();
    }
  }

  if(multipliers_size_one)
  {
    vector<SwitchNeg<T1> > mult_vars(vec.size());
    for(unsigned int i = 0; i < vec.size(); ++i)
      mult_vars[i] = SwitchNeg<T1>(vec[i], scale[i]);
    return BuildCT_GEQSUM(stateObj, mult_vars, t2, b);
  }
  else
  {
    vector<MultiplyVar<T1> > mult_vars(vec.size());
    for(unsigned int i = 0; i < vec.size(); ++i)
      mult_vars[i] = MultiplyVar<T1>(vec[i], scale[i]);
    return BuildCT_GEQSUM(stateObj, mult_vars, t2, b);
  }
}

#endif
