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

/** @help constraints;sumleq Description
The constraint

   sumleq(vec, c)

ensures that sum(vec) <= c.
*/

/** @help constraints;sumgeq Description
The constraint

   sumgeq(vec, c)

ensures that sum(vec) >= c.
*/ 

// This is the standard implementation of sumleq (and sumgeq)

#ifndef CONSTRAINT_FULLSUM_H
#define CONSTRAINT_FULLSUM_H


#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

/// V1 + ... Vn <= X
/// is_reversed checks if we are in the case where reverse_constraint was previously called.
template<typename VarArray, typename VarSum, BOOL is_reversed = false>
struct LessEqualSumConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "<=Sumup"; }
  
  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;
  
  bool no_negatives;
    
  VarArray var_array;  
  VarSum var_sum;
  Reversible<DomainInt> max_looseness;
  Reversible<DomainInt> var_array_min_sum;
  LessEqualSumConstraint(StateObj* _stateObj, const VarArray& _var_array, VarSum _var_sum) :
    AbstractConstraint(_stateObj), var_array(_var_array), var_sum(_var_sum), max_looseness(_stateObj), 
    var_array_min_sum(_stateObj)
  {
    no_negatives = true;
    for(int i = 0; i < var_array.size(); ++i)
    {
      if(var_array[i].getInitialMin() < 0)
      {
        no_negatives = false;
        return;
      }
    }
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
    {
	  t.push_back(make_trigger(var_array[i], Trigger(this, i), LowerBound));
    }
	t.push_back(make_trigger(var_sum, Trigger(this, -1), UpperBound));
    return t;    
  }
  
  DomainInt get_real_min_sum()
  {
	DomainInt min_sum = 0;
	for(typename VarArray::iterator it = var_array.begin(); it != var_array.end(); ++it)
      min_sum += it->getMin();
    return min_sum;
  }
  
  DomainInt get_real_max_diff()
  {
	DomainInt max_diff = 0;
    for(typename VarArray::iterator it = var_array.begin(); it != var_array.end(); ++it)
	  max_diff = max(max_diff, it->getMax() - it->getMin());
    return max_diff;
  }
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta domain_change)
  {
	PROP_INFO_ADDONE(FullSum);
    DomainInt sum = var_array_min_sum;
    if(prop_val != -1)
    { // One of the array changed
      int change = var_array[prop_val].getDomainChange(domain_change);
	  D_ASSERT(change >= 0);
	  sum += change;
      var_array_min_sum = sum;
    }
	
	var_sum.setMin(sum);
	if(getState(stateObj).isFailed())
        return;
	D_ASSERT(sum <= get_real_min_sum());
	
	DomainInt looseness = var_sum.getMax() - sum;
	if(looseness < 0)
	{ 
	  getState(stateObj).setFailed(true);
	  return;
	}

	if(looseness < max_looseness)
	{
	  // max_looseness.set(looseness);
	  for(typename VarArray::iterator it = var_array.begin(); it != var_array.end(); ++it)
	    it->setMax(it->getMin() + looseness);
	}
  }
  
  virtual BOOL check_unsat(int prop_val, DomainDelta domain_change)
  {
    DomainInt sum = var_array_min_sum;
    if(prop_val != -1)
    { // One of the array changed
      sum += var_array[prop_val].getDomainChange(domain_change);
      var_array_min_sum = sum;
    }
    return var_sum.getMax() < sum;
  }
  
  virtual BOOL full_check_unsat()
  {
    DomainInt min_sum = get_real_min_sum();
    DomainInt max_diff = get_real_max_diff();
 	
    var_array_min_sum = min_sum;
    max_looseness = max_diff;
    if(!var_array.empty())
      return check_unsat(0,0);
    else
    { 
      // Just set sum = 0 from check_unsat
      return var_sum.getMax() < 0;
    }
  }
  
  virtual void full_propagate()
  {
    DomainInt min_sum = 0;
    DomainInt max_diff = 0;
    for(typename VarArray::iterator it = var_array.begin(); it != var_array.end(); ++it)
    {
      min_sum += it->getMin();
      max_diff = max(max_diff, it->getMax() - it->getMin());
    }
	
    var_array_min_sum = min_sum;
    D_ASSERT(min_sum == get_real_min_sum());
    max_looseness = max_diff;
    if(!var_array.empty())
      propagate(0,0);
    else
      var_sum.setMin(0);
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array.size() + 1);
    DomainInt sum = 0;
    for(int i = 0; i < v_size - 1; i++)
      sum += v[i];
    return sum <= *(v + v_size - 1);
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    P("GSA");
    int sum_value = 0;
    int v_size = var_array.size();
    
    if(no_negatives)
    {
      int max_sum = var_sum.getMax();
      assignment.push_back(make_pair(v_size, max_sum));
      for(int i = 0; i < v_size && sum_value <= max_sum; ++i)
      {
        int min_val = var_array[i].getMin();
        assignment.push_back(make_pair(i, min_val));
        sum_value += min_val;
      }
      P("A" << (sum_value <= max_sum));
      return (sum_value <= max_sum);
    }
    else
    {
      for(int i = 0; i < v_size; ++i)
      {
        assignment.push_back(make_pair(i, var_array[i].getMin()));
        sum_value += var_array[i].getMin();
      }
      P("B" << (sum_value <= var_sum.getMax()));
      if(sum_value > var_sum.getMax())
        return false;
      else
        assignment.push_back(make_pair(v_size, var_sum.getMax()));
      return true;
    }
  }
  
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> array_copy(var_array.size() + 1);
	for(unsigned i = 0; i < var_array.size(); ++i)
	  array_copy[i] = var_array[i];
	array_copy[var_array.size()] = var_sum;
	return array_copy;
  }


  // All the code below here is to get around an annoying problem in C++. Basically we want to say that the
  // reverse of a <= is a >= constraint. However, when compiling C++ keeps getting the reverse of the reverse of..
  // and doesn't figure out it is looping. This code ensures we only go once around the loop.
  virtual AbstractConstraint* reverse_constraint()
  { return reverse_constraint_helper<is_reversed,int>::fun(stateObj, *this); }

// BUGFIX: The following two class definitions have a 'T=int' just to get around a really stupid parsing bug
// in g++ 4.0.x. Hopefully eventually we'll be able to get rid of it.

/// These classes are just here to avoid infinite recursion when calculating the reverse of the reverse
/// of a constraint.
  template<BOOL reversed, typename T>
	struct reverse_constraint_helper	
  {
    static AbstractConstraint* fun(StateObj* stateObj, LessEqualSumConstraint& con)
    {
	  typename NegType<VarArray>::type new_var_array(con.var_array.size());
	  for(unsigned i = 0; i < con.var_array.size(); ++i)
		new_var_array[i] = VarNegRef(con.var_array[i]);
	  
	  typedef typename ShiftType<typename NegType<VarSum>::type, compiletime_val<-1> >::type SumType;
	  SumType new_sum = ShiftVarRef( VarNegRef(con.var_sum), compiletime_val<-1>());
	  
	  return new LessEqualSumConstraint<typename NegType<VarArray>::type, SumType, true>
		(stateObj, new_var_array, new_sum);	
    }
  };
  
  template<typename T>
	struct reverse_constraint_helper<true, T>
  {
    static AbstractConstraint* fun(StateObj*, LessEqualSumConstraint&)
    { 
	  // This should never be reached, unless we try reversing an already reversed constraint.
	  // We have this code here as the above case makes templates, which if left would keep instansiating
	  // recursively and without bound.
	  FAIL_EXIT();
    }
  };
  
};





#endif
