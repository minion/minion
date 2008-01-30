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


/** Constraints two vectors of variables to be not equal.
 *
 *  \ingroup Constraints
 */
template<typename VarArray1, typename VarArray2>
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
  
  BOOL only_one_possible_pair;

  Reversible<bool> propagate_mode;
  int index_to_propagate; 

  VecNeqDynamic(StateObj* _stateObj,const VarArray1& _array1,
				const VarArray2& _array2) :
				DynamicConstraint(_stateObj), var_array1(_array1), var_array2(_array2),
				propagate_mode(false)
  { D_ASSERT(var_array1.size() == var_array2.size()); }
  
  int dynamic_trigger_count()
  { return 4; }
    
  template<typename Var>
  void remove_value(DomainInt val, Var& var)
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
  
  virtual void full_propagate()
  {
    D_INFO(2, DI_VECNEQ, "Starting full propagate");
	DynamicTrigger* dt = dynamic_trigger_start();
	int size = var_array1.size();
    int index = 0;
	
	// Find first pair we could watch.
	
	while(index < size &&
		  var_array1[index].isAssigned() && var_array2[index].isAssigned() &&
		  var_array1[index].getAssignedValue() == 
		  var_array2[index].getAssignedValue())
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
	while(index < size &&
		  var_array1[index].isAssigned() && var_array2[index].isAssigned() &&
		  var_array1[index].getAssignedValue() == 
		  var_array2[index].getAssignedValue())
	  ++index;
	
	// There is only one possible pair allowed...
	if(index == size)
	{
	  D_INFO(2, DI_VECNEQ, "Only found one possible: " + to_string(watched_index0));
	  if(var_array1[watched_index0].isAssigned())
	    remove_value(var_array1[watched_index0].getAssignedValue(),
					 var_array2[watched_index0]);
	  if(var_array2[watched_index0].isAssigned())
	    remove_value(var_array2[watched_index0].getAssignedValue(),
					 var_array1[watched_index0]);
	  only_one_possible_pair = true;
	  var_array1[watched_index0].addDynamicTrigger(dt, Assigned);
	  var_array2[watched_index0].addDynamicTrigger(dt + 1, Assigned);
	  return;
	}
	
	only_one_possible_pair = false;
	
	watched_index1 = index;

	D_INFO(2, DI_VECNEQ, "Found two indices: " + to_string(watched_index0) +
						 " and " + to_string(watched_index1));
	
	var_array1[watched_index0].addDynamicTrigger(dt, Assigned);
	var_array2[watched_index0].addDynamicTrigger(dt + 1, Assigned);
	var_array1[watched_index1].addDynamicTrigger(dt + 2, Assigned);
	var_array2[watched_index1].addDynamicTrigger(dt + 3, Assigned);
  }
  
  // XXX : I'm not sure this gets GAC, but it does some pruning, and is fast.
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
	PROP_INFO_ADDONE(DynVecNeq);
    D_INFO(2, DI_VECNEQ, "Starting propagate");
    if(only_one_possible_pair)
	{
	  D_INFO(2, DI_VECNEQ, "Only one pair: "+to_string(watched_index0));
	  if(var_array1[watched_index0].isAssigned())
	    remove_value(var_array1[watched_index0].getAssignedValue(),
					 var_array2[watched_index0]);
	  if(var_array2[watched_index0].isAssigned())
	    remove_value(var_array2[watched_index0].getAssignedValue(),
					 var_array1[watched_index0]);
	  return;
	}
	
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
		  D_ASSERT(var_array1[index_to_propagate].isAssigned());
		  remove_value(var_array1[index_to_propagate].getAssignedValue(),
			           var_array2[index_to_propagate]);
	    }
	    else
	    {  
		  D_ASSERT(var_array2[index_to_propagate].isAssigned());
		  remove_value(var_array2[index_to_propagate].getAssignedValue(),
			           var_array1[index_to_propagate]);
	    }
		return;		
	}
	

    // Check if the assignment has not caused a neq failure.
    if(triggerarray == 1)
    {
	  D_ASSERT(var_array1[original_index].isAssigned())
	  if(!var_array2[original_index].isAssigned() ||
		  var_array2[original_index].getAssignedValue() != var_array1[original_index].getAssignedValue())
		return;
    }
    else
    {  
	  D_ASSERT(var_array2[original_index].isAssigned())
	  if(!var_array1[original_index].isAssigned() ||
		  var_array1[original_index].getAssignedValue() != var_array2[original_index].getAssignedValue())
	    return;
    }
	
	int index = original_index + 1;
			   
	int size = var_array1.size();
	
	while( (index < size &&
		    var_array1[index].isAssigned() && var_array2[index].isAssigned() &&
			var_array1[index].getAssignedValue() == 
		    var_array2[index].getAssignedValue()) ||  index == other_index )
	  ++index;
	  
	if(index == size)
	{
	  index = 0;
	  while( (index < original_index &&
		      var_array1[index].isAssigned() && var_array2[index].isAssigned() &&
		      var_array1[index].getAssignedValue() == 
		      var_array2[index].getAssignedValue()) || index == other_index )
	  ++index;
	
	  if(index == original_index)
	  {// This is the only possible non-equal index.
	    D_INFO(2, DI_VECNEQ, "Cannot find another index");
	    propagate_mode = true;
		index_to_propagate = other_index;
		if(var_array2[other_index].isAssigned())
		  remove_value(var_array2[other_index].getAssignedValue(),
		               var_array1[other_index]);
		if(var_array1[other_index].isAssigned())
		  remove_value(var_array1[other_index].getAssignedValue(),
					   var_array2[other_index]);
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
	var_array1[index].addDynamicTrigger(trigs + triggerpair * 2, Assigned);
	var_array2[index].addDynamicTrigger(trigs + 1 + triggerpair * 2, Assigned);
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    int v_size1 = var_array1.size();
	return !std::equal(v.begin(), v.begin() + v_size1, v.begin() + v_size1);
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
	vars.reserve(var_array1.size() * var_array2.size());
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

