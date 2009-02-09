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

#ifndef CONSTRAINT_SUM_H_FGHJ
#define CONSTRAINT_SUM_H_FGHJ

// VarToCount = 1 means leq, = 0 means geq.
template<typename VarArray, typename VarSum, int VarToCount = 1 >
struct BoolLessSumConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { if(VarToCount) return "Bool<=Sum"; else return "Bool>=Sum"; }
  
  typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;
  
  ReversibleInt count;
  VarArray var_array;
  
  VarSum var_sum;
  
  BoolLessSumConstraint(StateObj* _stateObj, const VarArray& _var_array, VarSum _var_sum) :
    AbstractConstraint(_stateObj), count(_stateObj), var_array(_var_array), var_sum(_var_sum)
  { CHECK((VarToCount == 0) || (VarToCount == 1), "Fatal Internal Bug"); }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    int array_size = var_array.size();
	
    count = 0;    
    
    for(int i = 0; i < array_size; ++i)
      if(VarToCount)
	  	t.push_back(make_trigger(var_array[i], Trigger(this, i), LowerBound));
      else
	  	t.push_back(make_trigger(var_array[i], Trigger(this, i), UpperBound));
    return t;
  }
  
  virtual AbstractConstraint* reverse_constraint()
  { 
    if(VarToCount)
      return new BoolLessSumConstraint<VarArray, runtime_val, 0>(stateObj, var_array, runtime_val(var_sum + 1)); 
    else
      return new BoolLessSumConstraint<VarArray, runtime_val, 1>(stateObj, var_array, runtime_val(var_sum - 1));
  }
  
  int occ_count()
  {
    if (VarToCount)
      return var_sum;
    else
      return var_array.size() - var_sum;
  }
  
  
  void limit_reached()
  {
    int one_vars = 0;
    typename VarArray::value_type* it = &*var_array.begin();
    typename VarArray::value_type* end_it = it + var_array.size();
    for(; it < end_it; ++it)
    {
      if(it->isAssigned())
      { if(it->getAssignedValue() == VarToCount) ++one_vars; }
      else
      { it->uncheckedAssign(1 - VarToCount); }
    }
    //D_ASSERT(one_vars >= occ_count());
    if(one_vars > occ_count())
      getState(stateObj).setFailed(true);
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
	PROP_INFO_ADDONE(BoolSum);
    D_ASSERT(var_array[i].getAssignedValue() == 0 ||
			 var_array[i].getAssignedValue() == 1);
    int c = count + 1;
    count = c;
    if(c == occ_count())
      limit_reached();
  }
  
  virtual BOOL full_check_unsat()
  {
    int occs = 0;
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
      if(var_array[i].isAssignedValue(VarToCount))
		occs++;
    count = occs;
    if(occs > occ_count())
      return true;
    return false;
  }
  
  virtual BOOL check_unsat(int, DomainDelta)
  {
    int i = count + 1;
    count = i;
    if(i > occ_count())
      return true;
    else
      return false;
  }
  
  virtual void full_propagate()
  {
    int occs = 0;
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
      if(var_array[i].isAssignedValue(VarToCount))
		occs++;
    count = occs;
    if(occs > occ_count())
      getState(stateObj).setFailed(true);
    if(occs == occ_count())
      limit_reached();  
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array.size());
    for(int i = 0; i < v_size; i++)
      D_ASSERT(v[i] == 0 || v[i] == 1);
    if(VarToCount)
      return std::accumulate(v, v + v_size, DomainInt(0)) <= var_sum;
    else
      return std::accumulate(v, v + v_size, DomainInt(0)) >= var_sum;
  }
  
  /*
  // TODO : Optimise for booleans
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    int sum_value = 0;
    int v_size = var_array.size();
    if(VarToCount)
    {
      for(int i = 0; i < v_size; ++i)
      {
        assignment.push_back(make_pair(i, var_array[i].getMin()));
        sum_value += var_array[i].getMin();
      }
      return (sum_value <= var_sum);
    }
    else
    {
      for(int i = 0; i < v_size; ++i)
      {
        assignment.push_back(make_pair(i, var_array[i].getMax()));
        sum_value += var_array[i].getMax();
      }
      return (sum_value >= var_sum);
    }
  }
  */
  
  // TODO : Optimise for booleans
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    int v_size = var_array.size();
    int sum_limit;
    if(VarToCount)
      sum_limit = var_array.size() - var_sum;
    else
      sum_limit = var_sum;

    int ValToFind = 1 - VarToCount;

    int val_count = 0;
    
    for(int i = 0; i < v_size && val_count < sum_limit; ++i)
    {
      if(var_array[i].inDomain(ValToFind))
      {
        val_count++;
        assignment.push_back(make_pair(i, ValToFind));
      }
    }
    return val_count >= sum_limit;
  }
  
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
	vars.reserve(var_array.size());
	for(unsigned i = 0; i < var_array.size(); ++i)
	  vars.push_back(var_array[i]);
	return vars;
  }
};

template<typename VarArray,  typename VarSum>
AbstractConstraint*
BoolLessEqualSumCon(StateObj* stateObj, const VarArray& _var_array,  VarSum _var_sum)
{ 
  return (new BoolLessSumConstraint<VarArray,VarSum>(stateObj, _var_array,_var_sum)); 
}

template<typename VarArray,  typename VarSum>
AbstractConstraint*
BoolGreaterEqualSumCon(StateObj* stateObj, const VarArray& _var_array,  VarSum _var_sum)
{ 
  return (new BoolLessSumConstraint<VarArray,VarSum,0>(stateObj, _var_array,_var_sum)); 
}

#endif
