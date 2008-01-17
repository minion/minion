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


// This is the "primary" implement of sum, so it has to include headers for the
// other specialised implementations.

#include "constraint_lightsum.h"
#include "constraint_sum.h"


/// V1 + ... Vn <= X
/// is_reversed checks if we are in the case where reverse_constraint was previously called.
template<typename VarArray, typename VarSum, BOOL is_reversed = false>
struct LessEqualSumConstraint : public Constraint
{
  virtual string constraint_name()
  { return "<=Sumup"; }
  
  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;
  
  VarArray var_array;  
  VarSum var_sum;
  Reversible<DomainInt> max_looseness;
  Reversible<DomainInt> var_array_min_sum;
  LessEqualSumConstraint(const VarArray& _var_array, VarSum _var_sum) :
    var_array(_var_array), var_sum(_var_sum)
  { }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
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
	PropInfoAddone("FullSum");
    DomainInt sum = var_array_min_sum;
    if(prop_val != -1)
    { // One of the array changed
      int change = var_array[prop_val].getDomainChange(domain_change);
	  D_ASSERT(change >= 0);
	  sum += change;
      var_array_min_sum = sum;
    }
	
	var_sum.setMin(sum);
	D_ASSERT(sum <= get_real_min_sum());
	
	DomainInt looseness = var_sum.getMax() - sum;
	if(looseness < 0)
	{ 
	  Controller::fail();
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
	D_ASSERT(var_array[0].getDomainChange(0) == 0);
    return check_unsat(0,0);
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
    max_looseness = max_diff;
	D_ASSERT(var_array[0].getDomainChange(0) == 0);
    propagate(0,0);
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    D_ASSERT(v.size() == var_array.size() + 1);
    DomainInt sum = 0;
    int v_size = v.size();
    for(int i = 0; i < v_size - 1; i++)
      sum += v[i];
    return sum <= v.back();
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> array_copy(var_array.size() + 1);
	for(unsigned i = 0; i < var_array.size(); ++i)
	  array_copy[i] = var_array[i];
	array_copy[var_array.size()] = var_sum;
	return array_copy;
  }


  virtual Constraint* reverse_constraint()
  { return reverse_constraint_helper<is_reversed,int>::fun(*this); }

// BUGFIX: The following two class definitions have a 'T=int' just to get around a really stupid parsing bug
// in g++ 4.0.x. Hopefully eventually we'll be able to get rid of it.

/// These classes are just here to avoid infinite recursion when calculating the reverse of the reverse
/// of a constraint.
  template<BOOL reversed, typename T>
	struct reverse_constraint_helper	
  {
    static Constraint* fun(LessEqualSumConstraint& con)
    {
	  typename NegType<VarArray>::type new_var_array(con.var_array.size());
	  for(unsigned i = 0; i < con.var_array.size(); ++i)
		new_var_array[i] = VarNegRef(con.var_array[i]);
	  
	  typedef typename ShiftType<typename NegType<VarSum>::type, compiletime_val<-1> >::type SumType;
	  SumType new_sum = ShiftVarRef( VarNegRef(con.var_sum), compiletime_val<-1>());
	  
	  return new LessEqualSumConstraint<typename NegType<VarArray>::type, SumType, true>
		(new_var_array, new_sum);	
    }
  };
  
  template<typename T>
	struct reverse_constraint_helper<true, T>
  {
    static Constraint* fun(LessEqualSumConstraint&)
    { 
	  // This should never be reached, unless we try reversing an already reversed constraint.
	  // We have this code here as the above case makes templates, which if left would keep instansiating
	  // recursively and without bound.
	  FAIL_EXIT();
    }
  };
  
};



template<typename VarArray,  typename VarSum>
Constraint*
LessEqualSumCon(const VarArray& _var_array, const light_vector<VarSum>& _var_sum)
{ 
  if(_var_array.size() == 2)
  {
    array<typename VarArray::value_type, 2> v_array;
	for(int i = 0; i < 2; ++i)
	  v_array[i] = _var_array[i];
	return LightLessEqualSumCon(v_array, _var_sum[0]);
  }
  else
  {
	return new LessEqualSumConstraint<VarArray, VarSum>(_var_array, _var_sum[0]); 
  }
}

#ifndef NO_SPECIALISATIONS

inline Constraint*
LessEqualSumCon(const light_vector<BoolVarRef>& var_array, const light_vector<ConstantVar>& var_sum)
{ 
  runtime_val t2(checked_cast<int>(var_sum[0].getAssignedValue()));
  return BoolLessEqualSumCon(var_array, t2);
}

#endif

BUILD_CONSTRAINT2(CT_LEQSUM, LessEqualSumCon);



template<typename VarArray,  typename VarSum>
Constraint*
GreaterEqualSumCon(const VarArray& _var_array, const light_vector<VarSum>& _var_sum)
{ 
  if(_var_array.size() == 2)
  {
    array<typename VarArray::value_type, 2> v_array;
	for(int i = 0; i < 2; ++i)
	  v_array[i] = _var_array[i];
	return LightGreaterEqualSumCon(v_array, _var_sum[0]);
  }
  else
  {
	return (new LessEqualSumConstraint<typename NegType<VarArray>::type, 
			typename NegType<VarSum>::type>(VarNegRef(_var_array), VarNegRef(_var_sum[0]))); 
  }
}

#ifndef NO_SPECIALISATIONS


inline Constraint*
GreaterEqualSumCon(const light_vector<BoolVarRef>& var_array, const light_vector<ConstantVar>& var_sum)
{ 
  runtime_val t2(checked_cast<int>(var_sum[0].getAssignedValue()));
  return BoolGreaterEqualSumCon(var_array, t2);
}

#endif

BUILD_CONSTRAINT2(CT_GEQSUM, GreaterEqualSumCon);

