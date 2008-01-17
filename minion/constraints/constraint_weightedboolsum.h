/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "constraint_fullsum.h"

template<typename VarArray, typename WeightArray, typename VarSum>
struct LeqWeightBoolSumConstraint : public Constraint
{
  virtual string constraint_name()
  { return "LeqWeightBoolSum"; }
  
  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;
  
  VarSum var_sum;
  VarArray var_array;  
  WeightArray weight_array;
  //ReversibleInt max_looseness;
  ReversibleInt var_array_min_sum;
  ReversibleInt min_vals_weight_pos;
  
  typedef pair<const typename WeightArray::value_type*, const typename VarArray::value_type*> pair_val;
  
  struct compare_object
  {
    BOOL operator()(pair_val a, pair_val b)
  { return *(a.first) > *(b.first); }
  };
  
  LeqWeightBoolSumConstraint(const VarArray& _var_array, const WeightArray& _weight_array, const VarSum& _var_sum) :
    var_sum(_var_sum), var_array(_var_array.size()), weight_array(_weight_array.size())
  { 
      D_ASSERT(_var_array.size() == _weight_array.size());
      int array_size = _var_array.size();
      vector<pair_val> sort_array;
      for(int i = 0; i < array_size; ++i)
	sort_array.push_back(make_pair(&_weight_array[i], &_var_array[i]));
      std::sort(sort_array.begin(), sort_array.end(), compare_object());
      for(int i = 0; i < array_size; ++i)
      {
	var_array[i] = *(sort_array[i].second);
	weight_array[i] = *(sort_array[i].first);
      }
      for(int i = 0; i < array_size - 1; ++i)
	D_ASSERT(weight_array[i] >= weight_array[i+1]);
      min_vals_weight_pos = 0;
  }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
    
    int array_size = var_array.size();
    // A + B + C <= D
    for(int i = 0; i < array_size; ++i)
    {
	  t.push_back(make_trigger(var_array[i], Trigger(this,i), LowerBound));
    }
	t.push_back(make_trigger(var_sum, Trigger(this, -1), UpperBound));
    return t;
    
  }

  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
  {
	PROP_INFO_ADDONE(WeightBoolSum);
    int min_sum = var_array_min_sum;
    if(prop_val != -1)
    {
      min_sum += weight_array[prop_val];	
      var_sum.setMin(min_sum);
      var_array_min_sum = min_sum;
    }
    
    int slack_on_mins = var_sum.getMax() - min_sum;
    int weight_pos = min_vals_weight_pos;
    int weight_length = weight_array.size();
    while(weight_pos != weight_length && weight_array[weight_pos] > slack_on_mins)
    {
      if(!var_array[weight_pos].isAssigned())
	var_array[weight_pos].uncheckedAssign(0);
      weight_pos++;
    }
    min_vals_weight_pos = weight_pos;
  }
  
  virtual BOOL check_unsat(int, DomainDelta)
  {
    FAIL_EXIT();
  }
  
  virtual void full_propogate()
  {
    int min_sum = 0;
    
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
    {
      min_sum += var_array[i].getMin() * weight_array[i];
    }
    var_array_min_sum = min_sum;
    propogate(-1,0);
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    D_ASSERT(v.size() == var_array.size() + 1);
    int sum = 0;
    int v_size = v.size();
    for(int i = 0; i < v_size - 1; i++)
      sum += v[i];
    return sum <= v.back();
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
	vars.reserve(var_array.size() + 1);
    for(unsigned i = 0; i < var_array.size(); ++i)
	  vars.push_back(var_array[i]);
	vars.push_back(var_sum);
	return vars;
  }
  
};

template<typename VarArray, typename WeightArray, typename VarSum>
Constraint*
LeqWeightBoolSumCon(const VarArray& _var_array, const WeightArray& w_array, const VarSum& _var_sum)
{ return new LeqWeightBoolSumConstraint<VarArray, WeightArray, VarSum>(_var_array, w_array, _var_sum); }


template<typename VarArray, typename WeightArray, typename VarSum>
Constraint*
GeqWeightBoolSumCon(const VarArray& _var_array, const WeightArray& w_array, const VarSum& _var_sum)
{ 
  WeightArray rev_w_array(w_array.size());
  for(unsigned int i = 0; i < w_array.size(); ++i)
	rev_w_array[i] = -w_array[i];
	 
  return new LeqWeightBoolSumConstraint<VarArray, WeightArray, VarNeg<VarSum> >
	(_var_array, rev_w_array, VarNegRef(_var_sum)); 
}
 
template<typename T1, typename T2>
Constraint*
LeqWeightedSum(light_vector<int> scale, light_vector<T1> vec, const light_vector<T2>& t2)
{
  
  // Preprocess to remove any multiplications by 0, both for efficency
  // and correctness
  
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
	light_vector<SwitchNeg<T1> > mult_vars(vec.size());
	for(unsigned int i = 0; i < vec.size(); ++i)
	  mult_vars[i] = SwitchNeg<T1>(vec[i], scale[i]);
	return LessEqualSumCon(mult_vars, t2);
  }
  else
  {
	light_vector<MultiplyVar<T1> > mult_vars(vec.size());
	for(unsigned int i = 0; i < vec.size(); ++i)
	  mult_vars[i] = MultiplyVar<T1>(vec[i], scale[i]);
	return LessEqualSumCon(mult_vars, t2);
  }
}

// Don't pass in the vectors by reference, as we might need to copy them.
template<typename T1, typename T2>
Constraint*
GeqWeightedSum(light_vector<int> scale, light_vector<T1> vec, const light_vector<T2>& t2)
{
  
  // Preprocess to remove any multiplications by 0, both for efficency
  // and correctness
  
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
	light_vector<SwitchNeg<T1> > mult_vars(vec.size());
	for(unsigned int i = 0; i < vec.size(); ++i)
	  mult_vars[i] = SwitchNeg<T1>(vec[i], scale[i]);
	return GreaterEqualSumCon(mult_vars, t2);
  }
  else
  {
	light_vector<MultiplyVar<T1> > mult_vars(vec.size());
	for(unsigned int i = 0; i < vec.size(); ++i)
	  mult_vars[i] = MultiplyVar<T1>(vec[i], scale[i]);
	return GreaterEqualSumCon(mult_vars, t2);
  }
}

// XXX : This doesn't work at present. Just use general case.
/*
template<typename T1>
Constraint*
LeqWeightedSum(const vector<int>& scale, const vector<BoolVarRef>& vec, const vector<T1>& t2)
{ return LeqWeightBoolSumCon(vec, scale, t2[0]); }

template<typename T1>
Constraint*
GeqWeightedSum(const vector<int>& scale, const vector<BoolVarRef>& vec, const vector<T1>& t2)
{ return GeqWeightBoolSumCon(vec, scale, t2[0]); }
*/

BUILD_CONSTRAINT3(CT_WEIGHTLEQSUM, LeqWeightedSum)
BUILD_CONSTRAINT3(CT_WEIGHTGEQSUM, GeqWeightedSum)

