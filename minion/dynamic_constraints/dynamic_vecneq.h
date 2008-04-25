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

/** @help constraints;watchvecneq Description
The constraint

   watchvecneq(A, B)

ensures that A and B are not the same vector, i.e., there exists some index i
such that A[i] != B[i].
*/

/** @help constraints;watchvecneq Reifiability
This constraint is not reifiable.
*/

/** @help constraints;watchvecexists_less Description
The constraint

   watchvecexists_less(A, B)

ensures that there exists some index i such that A[i] < B[i].
*/

/** @help constraints;watchvecexists_less Reifiability
This constraint is not reifiable.
*/

/** @help constraints;watchvecexists_and Description
The constraint

   watchvecexists_less(A, B)

ensures that there exists some index i such that A[i] > 0 and B[i] > 0.

For booleans this is the same as 'exists i s.t. A[i] && B[i]'.
*/

/** @help constraints;watchvecexists_and Reifiability
This constraint is not reifiable.
*/

#ifndef _DYNAMIC_VECNEQ_H
#define _DYNAMIC_VECNEQ_H

struct NeqIterated
{
  static int dynamic_trigger_count()
    { return 2; }
  
  static bool check_assignment(DomainInt i, DomainInt j)
  { return i != j; }
  
  template<typename VarType1, typename VarType2>
  static bool no_support_for_pair(VarType1& var1, VarType2& var2)
  {
    return var1.isAssigned() && var2.isAssigned() &&
      var1.getAssignedValue() == var2.getAssignedValue();
  }
  
  template<typename VarType1, typename VarType2>  
  static void propagate_from_var1(VarType1& var1, VarType2& var2)
  {
    if(var1.isAssigned())
      remove_value(var1.getAssignedValue(), var2);
  }
  
  template<typename VarType1, typename VarType2>
  static void propagate_from_var2(VarType1& var1, VarType2& var2)
  {
    if(var2.isAssigned())
      remove_value(var2.getAssignedValue(), var1);
  }
  
  template<typename VarType1, typename VarType2>
  static void add_triggers(VarType1& var1, VarType2& var2, DynamicTrigger* dt)
  {
     var1.addDynamicTrigger(dt, Assigned);
     var2.addDynamicTrigger(dt + 1, Assigned);
  }
    
  template<typename Var>
  static void remove_value(DomainInt val, Var& var)
  {
    if(var.isBound())
    {
      if(var.getMin() == val)
        var.setMin(val + 1);
      else
        if(var.getMax() == val)
        var.setMax(val - 1);
    }
    else
      { var.removeFromDomain(val); }
  }
};

struct LessIterated
{
  static bool check_assignment(DomainInt i, DomainInt j)
  { return i < j; }
  
  static int dynamic_trigger_count()
  { return 2; }
  
  template<typename VarType1, typename VarType2>
  static bool no_support_for_pair(VarType1& var1, VarType2& var2)
  {
    return var1.getMin() >= var2.getMax();
  }
  
  template<typename VarType1, typename VarType2>  
  static void propagate_from_var1(VarType1& var1, VarType2& var2)
  {
    var2.setMin(var1.getMin() + 1);
  }
  
  template<typename VarType1, typename VarType2>
  static void propagate_from_var2(VarType1& var1, VarType2& var2)
  {
    var1.setMax(var2.getMax() - 1);
  }
  
  template<typename VarType1, typename VarType2>
  static void add_triggers(VarType1& var1, VarType2& var2, DynamicTrigger* dt)
  {
     var1.addDynamicTrigger(dt, LowerBound);
     var2.addDynamicTrigger(dt + 1, UpperBound);
  }
};

struct BothNonZeroIterated
{
  static bool check_assignment(DomainInt i, DomainInt j)
  { return i > 0 && j > 0; }

  static int dynamic_trigger_count()
  { return 2; }
  
  template<typename VarType1, typename VarType2>
  static bool no_support_for_pair(VarType1& var1, VarType2& var2)
  {
    return var1.getMax() <= 0 || var2.getMax() <= 0;
  }
  
  template<typename VarType1, typename VarType2>  
  static void propagate_from_var1(VarType1& var1, VarType2& var2)
  {
    var2.setMin(1);
  }
  
  template<typename VarType1, typename VarType2>
  static void propagate_from_var2(VarType1& var1, VarType2& var2)
  {
    var1.setMin(1);
  }
  
  template<typename VarType1, typename VarType2>
  static void add_triggers(VarType1& var1, VarType2& var2, DynamicTrigger* dt)
  {
     var1.addDynamicTrigger(dt, UpperBound);
     var2.addDynamicTrigger(dt + 1, UpperBound);
  }
};

/** Constraints two vectors of variables to be not equal.
  *
  *  \ingroup Constraints
*/
template<typename VarArray1, typename VarArray2, typename Operator = NeqIterated>
  struct VecNeqDynamic : public DynamicConstraint
{
  virtual string constraint_name()
    { return "VecNeqDynamic"; }

  typedef typename VarArray1::value_type VarRef1;
  typedef typename VarArray2::value_type VarRef2;

  VarArray1 var_array1;
  VarArray2 var_array2;

  int watched_index0;
  int watched_index1;

  Reversible<bool> propagate_mode;
  int index_to_propagate; 

  VecNeqDynamic(StateObj* _stateObj, const VarArray1& _array1,
    const VarArray2& _array2) :
  DynamicConstraint(_stateObj), var_array1(_array1), var_array2(_array2),
    propagate_mode(_stateObj, false)
    { D_ASSERT(var_array1.size() == var_array2.size()); }

  int dynamic_trigger_count()
    { return Operator::dynamic_trigger_count() * 2; }



  bool no_support_for_index(int index)
  { return Operator::no_support_for_pair(var_array1[index], var_array2[index]); }


  void add_triggers(int index, DynamicTrigger* dt)
  {
    Operator::add_triggers(var_array1[index], var_array2[index], dt);
  }
  
  virtual void full_propagate()
  {
    D_INFO(2, DI_VECNEQ, "Starting full propagate");
    DynamicTrigger* dt = dynamic_trigger_start();
    int size = var_array1.size();
    int index = 0;

    // Find first pair we could watch.
    while(index < size && no_support_for_index(index))
      ++index;

    // Vectors are assigned and equal.
    if(index == size)
    {
      getState(stateObj).setFailed(true);
      return;
    }

    watched_index0 = index;

    ++index; 

    // Now, is there another fine pair?
    while(index < size && no_support_for_index(index))
      ++index;

    // There is only one possible pair allowed...
    if(index == size)
    {
      D_INFO(2, DI_VECNEQ, "Only found one possible: " + to_string(watched_index0));      
      propagate_from_var1(watched_index0);
      propagate_from_var2(watched_index0);
      propagate_mode = true;
      index_to_propagate = watched_index0;
      add_triggers(watched_index0, dt);
      return;
    }

    watched_index1 = index;

    D_INFO(2, DI_VECNEQ, "Found two indices: " + to_string(watched_index0) +
      " and " + to_string(watched_index1));

    add_triggers(watched_index0, dt);
    add_triggers(watched_index1, dt + 2);
  }

  void propagate_from_var1(int index)
  { Operator::propagate_from_var1(var_array1[index], var_array2[index]); }
  
  void propagate_from_var2(int index)
  {  Operator::propagate_from_var2(var_array1[index], var_array2[index]); }

  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
    PROP_INFO_ADDONE(DynVecNeq);
    D_INFO(2, DI_VECNEQ, "Starting propagate");

    int trigger_activated = dt - dynamic_trigger_start();
    int triggerpair = trigger_activated / 2;
    D_ASSERT(triggerpair == 0 || triggerpair == 1);
    // Var arrays are numbered 1 and 2
    int triggerarray = (trigger_activated % 2) + 1;
    D_ASSERT(triggerarray == 1 || triggerarray == 2);

    int original_index;
    int other_index;

    if(triggerpair == 0)
    { 
      original_index = watched_index0;
      other_index = watched_index1;
    }
    else
    {
      original_index = watched_index1;
      other_index = watched_index0;
    }

    if(propagate_mode)
    {
      // If this is true, the other index got assigned.
      if(index_to_propagate != original_index)
        return;

      if(triggerarray == 1)
      {
        propagate_from_var1(index_to_propagate);
      }
      else
      {  
        propagate_from_var2(index_to_propagate);
      }
      return;   
    }

    // Check if propagation has caused a loss of support.
    if(!no_support_for_index(original_index))
      return;

    int index = original_index + 1;

    int size = var_array1.size();

    while( (index < size && no_support_for_index(index) ) || index == other_index )
      ++index;

    if(index == size)
    {
      index = 0;
      while( (index < original_index && no_support_for_index(index) ) || index == other_index )
        ++index;

      if(index == original_index)
      {
        // This is the only possible non-equal index.
        D_INFO(2, DI_VECNEQ, "Cannot find another index");
        propagate_mode = true;
        index_to_propagate = other_index;
        propagate_from_var1(other_index);
        propagate_from_var2(other_index);
        return;
      }
    }

    D_INFO(2, DI_VECNEQ, "Now going to watch " + to_string(index));

    if(triggerpair == 0)
      watched_index0 = index;
    else
      watched_index1 = index;

    D_ASSERT(watched_index0 != watched_index1);
    DynamicTrigger* trigs = dynamic_trigger_start();
    add_triggers(index, trigs + triggerpair * 2);
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    int v_size1 = var_array1.size();
    for(int i = 0; i < v_size1; ++i)
      if(Operator::check_assignment(v[i], v[i + v_size1]))
        return true;
    return false;
  }

  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(var_array1.size() + var_array2.size());
    for(unsigned i = 0; i < var_array1.size(); ++i)
      vars.push_back(AnyVarRef(var_array1[i]));
    for(unsigned i = 0; i < var_array2.size(); ++i)
      vars.push_back(AnyVarRef(var_array2[i]));
    return vars;  
  }
};

template<typename VarArray1,  typename VarArray2>
DynamicConstraint*
  VecNeqConDynamic(StateObj* stateObj,const VarArray1& varray1, const VarArray2& varray2)
  { return new VecNeqDynamic<VarArray1,VarArray2>(stateObj, varray1, varray2); }

BUILD_DYNAMIC_CONSTRAINT2(CT_WATCHED_VECNEQ, VecNeqConDynamic)

template<typename VarArray1,  typename VarArray2>
DynamicConstraint*
  VecOrLessConDynamic(StateObj* stateObj,const VarArray1& varray1, const VarArray2& varray2)
  { return new VecNeqDynamic<VarArray1,VarArray2, LessIterated>(stateObj, varray1, varray2); }

BUILD_DYNAMIC_CONSTRAINT2(CT_WATCHED_VEC_OR_LESS, VecOrLessConDynamic)

template<typename VarArray1,  typename VarArray2>
DynamicConstraint*
  VecOrAndConDynamic(StateObj* stateObj,const VarArray1& varray1, const VarArray2& varray2)
  { return new VecNeqDynamic<VarArray1,VarArray2, BothNonZeroIterated>(stateObj, varray1, varray2); }

BUILD_DYNAMIC_CONSTRAINT2(CT_WATCHED_VEC_OR_AND, VecOrAndConDynamic)

#endif
