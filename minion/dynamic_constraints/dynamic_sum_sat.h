/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: dynamic_sum.h 398 2006-10-17 09:49:19Z gentian $
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



// VarToCount = 1 means leq, = 0 means geq.
template<typename VarArray>
struct BoolSATConstraintDynamic : public DynamicConstraint
{
  virtual string constraint_name()
  { return "BoolSATDynamic"; }
  
  typedef typename VarArray::value_type VarRef;
  
  VarArray var_array;

  int last;
  
  BoolSATConstraintDynamic(const VarArray& _var_array) :
	var_array(_var_array)
  { 
#ifndef WATCHEDLITERALS
    cerr << "This almost certainly isn't going to work... sorry" << endl;
#endif
  }
  
  int dynamic_trigger_count()
  {
	last = 0;
	
	D_INFO(2,DI_DYSUMCON,"Setting up Dynamic Trigger Constraint for BOOLSATConstraintDynamic");
	return 2;
  }
    
  virtual void full_propagate()
  {
	DynamicTrigger* dt = dynamic_trigger_start();
	
	int array_size = var_array.size(); 
	int trig1, trig2;
	int index = 0;
	
	while(index < array_size && !var_array[index].inDomain(1))
      ++index;
	
	trig1 = index;

	if(index == array_size)
	{ // Not enough triggers
	  Controller::fail();
	  return;
	}
	
    ++index;
	
	while(index < array_size && !var_array[index].inDomain(1))
      ++index;
	
	trig2 = index;
	
	if(index >= array_size)
	{ // Only one valid variable.
	  var_array[trig1].propagateAssign(1);
	  return;
	}
	
    dt->trigger_info() = trig1;
	var_array[trig1].addDynamicTrigger(dt, UpperBound);
	
	++dt;
	
	dt->trigger_info() = trig2;
	var_array[trig2].addDynamicTrigger(dt, UpperBound);
	
	return;
  }
    
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
	PropInfoAddone("DynSumSat");
	int propval = dt->trigger_info();
	int var_size = var_array.size();
	
	DynamicTrigger* base_dt = dynamic_trigger_start();
	int other_propval;
	
	if(base_dt == dt)
	  other_propval = (base_dt + 1)->trigger_info();
	else
	  other_propval = base_dt->trigger_info();
	
// I thought this would make the code go faster. But it doesn't!
//	if(var_array[other_propval].isAssignedValue(1))
//	  return;
	
    D_INFO(1, DI_DYSUMCON, "Triggering on domain of "+ to_string(propval));

	bool found_new_support = false;

	int loop = last;
	
	while(loop < var_size && !found_new_support)
	{
	  if(loop != other_propval && var_array[loop].inDomain(1))
	    found_new_support = true;
	  else
		++loop;
	}
	
	
	if(!found_new_support)
	{
	  loop = 0;
	  
	  while(loop < last && !found_new_support)
	  {
		if(loop != other_propval && var_array[loop].inDomain(1))
		  found_new_support = true;
		else
		  ++loop;
	  }
	
	  if(!found_new_support)
	  {  // Have to propagate!
        var_array[other_propval].propagateAssign(1);
	    return;
	  }
	}
	
	// Found new value to watch
	dt->trigger_info() = loop;
	last = loop;
	var_array[loop].addDynamicTrigger(dt, UpperBound);
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    D_ASSERT(v.size() == var_array.size());
    int v_size = v.size();
	int count = 0;
	for(int i = 0; i < v_size; ++i)
	  count += (v[i] == 1);
	return count > 0;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
	vars.reserve(var_array.size());
	for(unsigned i = 0; i < var_array.size(); ++i)
	  vars.push_back(AnyVarRef(var_array[i]));
	return vars;  
  }
};

template<typename VarArray>
DynamicConstraint*
BoolSATConDynamic(const VarArray& _var_array)
{ return new BoolSATConstraintDynamic<VarArray>(_var_array); }
