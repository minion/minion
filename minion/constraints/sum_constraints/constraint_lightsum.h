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

#ifndef CONSTRAINT_LIGHTSUM_H_PLOKIJ
#define CONSTRAINT_LIGHTSUM_H_PLOKIJ

/// V1 + ... Vn <= X
template<typename VarRef, std::size_t size, typename VarSum, BOOL is_reversed = false>
struct LightLessEqualSumConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Light<=Sum"; }
  
  bool no_negatives;
  
  array<VarRef, size> var_array;  
  VarSum var_sum;
  LightLessEqualSumConstraint(StateObj* _stateObj, const array<VarRef, size>& _var_array, const VarSum& _var_sum) :
    AbstractConstraint(_stateObj), var_array(_var_array), var_sum(_var_sum)
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
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
    
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
    { t.push_back(make_trigger(var_array[i], Trigger(this, i), LowerBound)); }
    
    t.push_back(make_trigger(var_sum, Trigger(this, -1), UpperBound));
    return t;    
  }
  
  /*
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    int sum_value = 0;
    int v_size = var_array.size();
    for(int i = 0; i < v_size; ++i)
    {
      assignment.push_back(make_pair(i, var_array[i].getMin()));
      sum_value += var_array[i].getMin();
    }
    if(sum_value > var_sum.getMax())
      return false;
    else
      assignment.push_back(make_pair(v_size, var_sum.getMax()));
    return true;
  }
  */
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    int sum_value = 0;
    int v_size = var_array.size();
    
    if(no_negatives)
    {
      int max_sum = var_sum.getMax();
      assignment.push_back(make_pair(v_size, max_sum));
      for(int i = 0; i < v_size && sum_value < max_sum; ++i)
      {
        int min_val = var_array[i].getMin();
        assignment.push_back(make_pair(i, min_val));
        sum_value += min_val;
      }
      return (sum_value <= max_sum);
    }
    else
    {
      for(int i = 0; i < v_size; ++i)
      {
        assignment.push_back(make_pair(i, var_array[i].getMin()));
        sum_value += var_array[i].getMin();
      }
      if(sum_value > var_sum.getMax())
        return false;
      else
        assignment.push_back(make_pair(v_size, var_sum.getMax()));
      return true;
    }
  }
  
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
  {
	PROP_INFO_ADDONE(LightSum);
    DomainInt min_sum = 0;
    for(unsigned i = 0; i < size; ++i)
      min_sum += var_array[i].getMin();
    
    if(prop_val >= 0)
    { var_sum.setMin(min_sum); }
    
    DomainInt slack = var_sum.getMax() - min_sum;
    for(unsigned i = 0; i < size; ++i)
      var_array[i].setMax(var_array[i].getMin() + slack);
  }
  
  virtual BOOL full_check_unsat()
  { return check_unsat(0, 0); }
  
  virtual BOOL check_unsat(int, DomainDelta)
  {
    DomainInt min_sum = 0;
    for(unsigned i = 0; i < size; ++i)
      min_sum += var_array[i].getMin();
    return min_sum > var_sum.getMax();
  }
  
  virtual void full_propagate()
  {
    propagate(-1,0);
    propagate(0,0);
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array.size() + 1);
    DomainInt sum = 0;
    for(int i = 0; i < v_size - 1; i++)
      sum += v[i];
    return sum <= *(v + v_size - 1);
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> array_copy;
	array_copy.reserve(var_array.size() + 1);
    for(unsigned i = 0; i < var_array.size(); ++i)
      array_copy.push_back(var_array[i]);
    array_copy.push_back(var_sum);
    return array_copy;
  }

 virtual AbstractConstraint* reverse_constraint()
  { return reverse_constraint_helper<is_reversed,int>::fun(stateObj, *this); }

// BUGFIX: The following two class definitions have a 'T=int' just to get around a really stupid parsing bug
// in g++ 4.0.x. Hopefully eventually we'll be able to get rid of it.

/// These classes are just here to avoid infinite recursion when calculating the reverse of the reverse
/// of a constraint.
  template<BOOL reversed, typename T>
    struct reverse_constraint_helper    
  {
    static AbstractConstraint* fun(StateObj* stateObj,LightLessEqualSumConstraint& con)
    {
      typedef array<typename NegType<VarRef>::type, size> VarArray;
      VarArray new_var_array;
      for(unsigned i = 0; i < con.var_array.size(); ++i)
        new_var_array[i] = VarNegRef(con.var_array[i]);
      
      typedef typename ShiftType<typename NegType<VarSum>::type, compiletime_val<-1> >::type SumType;
      SumType new_sum = ShiftVarRef(VarNegRef(con.var_sum), compiletime_val<-1>());
      
      return new LightLessEqualSumConstraint<typename NegType<VarRef>::type, size, SumType, true>
        (stateObj, new_var_array, new_sum);   
    }
  };
  
  template<typename T>
    struct reverse_constraint_helper<true, T>
  {
    static AbstractConstraint* fun(StateObj*, LightLessEqualSumConstraint&)
    { 
      // This should never be reached, unless we try reversing an already reversed constraint.
      // We have this code here as the above case makes templates, which if left would keep instansiating
      // recursively and without bound.
      FAIL_EXIT();
    }
  };
  
};

template<typename VarRef, std::size_t size, typename VarSum>
AbstractConstraint*
LightLessEqualSumCon(StateObj* stateObj, const array<VarRef,size>& _var_array,  const VarSum& _var_sum)
{ 
  return (new LightLessEqualSumConstraint<VarRef, size, VarSum>(stateObj, _var_array,_var_sum)); 
}


template<typename VarRef, std::size_t size, typename VarSum>
AbstractConstraint*
LightGreaterEqualSumCon(StateObj* stateObj, const array<VarRef,size>& _var_array, const VarSum& _var_sum)
{ 
  return 
  (new LightLessEqualSumConstraint<typename NegType<VarRef>::type, size, 
   typename NegType<VarSum>::type>(stateObj, VarNegRef(_var_array), VarNegRef(_var_sum))); 
}

#endif
