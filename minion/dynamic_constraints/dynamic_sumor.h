/* Minion Constraint Solver
http://minion.sourceforge.net

For Licence Information see file LICENSE.txt 

  $Id: dynamic_vecneq.h 1117 2008-02-15 17:19:14Z caj $
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

/** @help constraints;hamming Description
The constraint

   hamming(X,Y,c)

ensures that the hamming distance between X and Y is at least c. That is, that
the size of the set {i | X[i] != y[i]} is greater than or equal to c.
*/

/** @help constraints;hamming Reifiability
This constraint is reifyimply'able but not reifiable.
*/

// For operators
#include "dynamic_vecneq.h"

/** Constraints two vectors of variables to be not equal.
  *
  *  \ingroup Constraints
*/
template<typename VarArray1, typename VarArray2, typename Operator = NeqIterated>
  struct VecCountDynamic : public AbstractConstraint
{
  virtual string constraint_name()
    { return "VecCountDynamic"; }

  typedef typename VarArray1::value_type VarRef1;
  typedef typename VarArray2::value_type VarRef2;

  VarArray1 var_array1;
  VarArray2 var_array2;
  int num_to_watch;
  vector<int> watched_values;
  vector<int> unwatched_values;

  Reversible<bool> propagate_mode;
  int index_to_not_propagate; 

  VecCountDynamic(StateObj* _stateObj, const VarArray1& _array1, const VarArray2& _array2, int _num_of_vals) :
  AbstractConstraint(_stateObj), var_array1(_array1), var_array2(_array2), num_to_watch(_num_of_vals + 1), 
    propagate_mode(_stateObj, false)
    {
       if(num_to_watch <= 1)
         num_to_watch = 0;
       D_ASSERT(var_array1.size() == var_array2.size()); 
    }

  int dynamic_trigger_count()
  { return Operator::dynamic_trigger_count() * num_to_watch; }

  bool no_support_for_index(int index)
  { return Operator::no_support_for_pair(var_array1[index], var_array2[index]); }


  void add_triggers(int index, DynamicTrigger* dt)
  {
    Operator::add_triggers(var_array1[index], var_array2[index], dt);
  }
  
  virtual void full_propagate()
  {
    D_INFO(2, DI_VECNEQ, "Starting full propagate");
    
    // Check if the constraint is trivial, if so just exit now.
    if(num_to_watch <= 1)
      return;
      
    watched_values.resize(num_to_watch);
      
    DynamicTrigger* dt = dynamic_trigger_start();
    int size = var_array1.size();
    int index = 0;
    int found_matches = 0;
    // Find first pair we could watch.
    
    while(found_matches < num_to_watch && index < size)
    {
      while(index < size && no_support_for_index(index))
      {
        ++index;
      }
      if(index != size)
      {
        watched_values[found_matches] = index;
        ++found_matches;
        ++index;
      }
    }
    
    // Failed to find enough watches
    if(found_matches < num_to_watch - 1)
    {
      getState(stateObj).setFailed(true);
      return;
    }

    // Found exactly as many values as we need to propagate
    if(found_matches == num_to_watch - 1)
    {
      propagate_mode = true;
      index_to_not_propagate = num_to_watch - 1;
      for(int i = 0; i < num_to_watch - 1; ++i)
      {
        propagate_from_var1(watched_values[i]);
        propagate_from_var2(watched_values[i]);
        add_triggers(watched_values[i], dt + 2*i);
      }
      return;
    }

    // Found enough values to watch, no propagation yet!
    for(int i = 0; i < num_to_watch; ++i)
    {
      add_triggers(watched_values[i], dt + 2*i);
    }
    
    // Setup the 'unwatched values' array.
    initalise_unwatched_values();
  }
  
  void initalise_unwatched_values()
  {
    unwatched_values.resize(0);
    for(int i = 0; i < var_array1.size(); ++i)
    {
      bool found = false;
      for(int j = 0; j < watched_values.size(); ++j)
      {
        if(i == watched_values[j])
          found = true;
      }
      if(!found)
        unwatched_values.push_back(i); 
    }
    
    random_shuffle(unwatched_values.begin(), unwatched_values.end());
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
    D_ASSERT(triggerpair >= 0 && triggerpair < num_to_watch);
/*
    printf("propmode=%d, triggerpair=%d, trigger_activated=%d, nopropindex=%d\n",
      (int)propagate_mode, (int)triggerpair, (int)trigger_activated, (int)index_to_not_propagate);

    for(int i = 0; i < watched_values.size(); ++i)
      printf("%d,", watched_values[i]);
    
    printf(":");  
    for(int i = 0; i < unwatched_values.size(); ++i)
      printf("%d,", unwatched_values[i]);
    printf("\n");
*/    
    if(propagate_mode)
    {
      // If this is true, the other index got assigned.
      if(index_to_not_propagate == watched_values[triggerpair])
        return;

      if(trigger_activated % 2 == 0)
      { propagate_from_var1(watched_values[triggerpair]); }
      else
      { propagate_from_var2(watched_values[triggerpair]); }
      return;   
    }

    // Check if propagation has caused a loss of support.
    if(!no_support_for_index(watched_values[triggerpair]))
      return;

    int index = 0;
    int unwatched_size = unwatched_values.size();
    while(index < unwatched_size && no_support_for_index(unwatched_values[index]))
      index++;
      
    if(index == unwatched_size)
    {
      // This is the only possible non-equal index.
      D_INFO(2, DI_VECNEQ, "Cannot find another index");
      propagate_mode = true;
      index_to_not_propagate = watched_values[triggerpair];
      
//     printf("!propmode=%d, triggerpair=%d, trigger_activated=%d, nopropindex=%d\n",
//        (int)propagate_mode, (int)triggerpair, (int)trigger_activated, (int)index_to_not_propagate);
        
      for(int i = 0; i < watched_values.size(); ++i)
      {
        if(i != triggerpair)
        {
          propagate_from_var1(watched_values[i]);
          propagate_from_var2(watched_values[i]);
        }
      }
      return;
    }

    swap(watched_values[triggerpair], unwatched_values[index]);
    
    DynamicTrigger* trigs = dynamic_trigger_start();
    add_triggers(watched_values[triggerpair], trigs + triggerpair * 2);
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    int v_size1 = var_array1.size();
    int count = 0;
    for(int i = 0; i < v_size1; ++i)
      if(Operator::check_assignment(v[i], v[i + v_size1]))
        count++;
    return (count >= (num_to_watch - 1));
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
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    if(num_to_watch <= 1)
    {
      // This constraint is always true. But we can't return an empty allowed assignment unfortunatly.
      assignment.push_back(make_pair(0, var_array1[0].getMin()));
      return true;
    }
    
    pair<int, int> assign;
    int found_satisfying = 0;
    for(int i = 0; i < var_array1.size(); ++i)
    {
      if(Operator::get_satisfying_assignment(var_array1[i], var_array2[i], assign))
      {
        found_satisfying++;
        D_ASSERT(var_array1[i].inDomain(assign.first));
        D_ASSERT(var_array2[i].inDomain(assign.second));
        D_ASSERT(Operator::check_assignment(assign.first, assign.second));
        assignment.push_back(make_pair(i, assign.first));
        assignment.push_back(make_pair(i + var_array1.size(), assign.second));
        if(found_satisfying == num_to_watch - 1)
          return true;
      }
    }
    // If we didn't get enough, 
    return false;
  }
  
};

template<typename VarArray1,  typename VarArray2>
AbstractConstraint*
  VecOrCountConDynamic(StateObj* stateObj,const VarArray1& varray1, const VarArray2& varray2, int i)
  { return new VecCountDynamic<VarArray1,VarArray2>(stateObj, varray1, varray2, i); }

//BUILD_DYNAMIC_CONSTRAINT2(CT_WATCHED_HAMMING, VecOrCountConDynamic)

template<typename T1, typename T2>
AbstractConstraint*
BuildCT_WATCHED_HAMMING(StateObj* stateObj, const T1& t1, const T2& t2, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b) 
{
  if(reify)
    { INPUT_ERROR("Can't reify a hamming constraint."); }
  else
  { return VecOrCountConDynamic(stateObj, t1, t2, b.constants[0][0]); }
}

/*
template<typename VarArray1,  typename VarArray2>
AbstractConstraint*
  VecOrLessConDynamic(StateObj* stateObj,const VarArray1& varray1, const VarArray2& varray2)
  { return new VecNeqDynamic<VarArray1,VarArray2, LessIterated>(stateObj, varray1, varray2); }

BUILD_DYNAMIC_CONSTRAINT2(CT_WATCHED_VEC_OR_LESS, VecOrLessConDynamic)

template<typename VarArray1,  typename VarArray2>
AbstractConstraint*
  VecOrAndConDynamic(StateObj* stateObj,const VarArray1& varray1, const VarArray2& varray2)
  { return new VecNeqDynamic<VarArray1,VarArray2, BothNonZeroIterated>(stateObj, varray1, varray2); }

BUILD_DYNAMIC_CONSTRAINT2(CT_WATCHED_VEC_OR_AND, VecOrAndConDynamic)
*/
