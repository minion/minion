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

#ifndef CONSTRAINT_DYNAMIC_SUM_OR_H
#define CONSTRAINT_DYNAMIC_SUM_OR_H

// For operators
#include "dynamic_vecneq.h"

template<typename VarArray1, typename VarArray2, typename Operator = NeqIterated, BOOL is_reversed = false>
  struct VecCountDynamic : public AbstractConstraint
{
  virtual string constraint_name()
    { return "VecCountDynamic"; }

  typedef typename VarArray1::value_type VarRef1;
  typedef typename VarArray2::value_type VarRef2;

  VarArray1 var_array1;
  VarArray2 var_array2;
  int num_to_watch;
  int hamming_distance;
  vector<int> watched_values;
  vector<int> unwatched_values;

  Reversible<bool> propagate_mode;
  int index_to_not_propagate; 

  VecCountDynamic(StateObj* _stateObj, const VarArray1& _array1, const VarArray2& _array2, int _hamming_distance) :
  AbstractConstraint(_stateObj), var_array1(_array1), var_array2(_array2), num_to_watch(_hamming_distance + 1), hamming_distance(_hamming_distance),
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
    int trigger_activated = dt - dynamic_trigger_start();
    int triggerpair = trigger_activated / Operator::dynamic_trigger_count();   
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
      return true;
    
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
  
  // All the code below here is to get around an annoying problem in C++. Basically we want to say that the
  // reverse of a <= is a >= constraint. However, when compiling C++ keeps getting the reverse of the reverse of..
  // and doesn't figure out it is looping. This code ensures we only go once around the loop.
  virtual AbstractConstraint* reverse_constraint()
  { return reverse_constraint_helper<is_reversed,typename Operator::reverse_operator>::fun(stateObj, var_array1, var_array2, hamming_distance); }

// BUGFIX: The following two class definitions have a 'T=int' just to get around a really stupid parsing bug
// in g++ 4.0.x. Hopefully eventually we'll be able to get rid of it.

/// These classes are just here to avoid infinite recursion when calculating the reverse of the reverse
/// of a constraint.
  template<BOOL reversed, typename T>
	struct reverse_constraint_helper	
  {
      static AbstractConstraint* fun(StateObj* stateObj, VarArray1 var_array1, VarArray2 var_array2, int hamming_distance)
    {
	  return new VecCountDynamic<VarArray1, VarArray2, T, true>
        (stateObj, var_array1, var_array2, var_array1.size()-hamming_distance+1);
    }
  };
  
  template<typename T>
	struct reverse_constraint_helper<true, T>
  {
    static AbstractConstraint* fun(StateObj*, VarArray1 var_array1, VarArray2 var_array2, int hamming_distance)
    { 
	  // This should never be reached, unless we try reversing an already reversed constraint.
	  // We have this code here as the above case makes templates, which if left would keep instansiating
	  // recursively and without bound.
	  FAIL_EXIT();
    }
  };
  
};

template<typename VarArray1,  typename VarArray2>
AbstractConstraint*
  VecOrCountConDynamic(StateObj* stateObj,const VarArray1& varray1, const VarArray2& varray2, int i)
  { return new VecCountDynamic<VarArray1,VarArray2>(stateObj, varray1, varray2, i); }

template<typename T1, typename T2>
AbstractConstraint*
BuildCT_WATCHED_HAMMING(StateObj* stateObj, const T1& t1, const T2& t2, ConstraintBlob& b) 
{ return VecOrCountConDynamic(stateObj, t1, t2, b.constants[0][0]); }


#endif

