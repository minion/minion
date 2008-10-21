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
struct BoolThreeSATConstraintDynamic : public AbstractConstraint
{
  virtual string constraint_name()
  { return "BoolSATDynamic"; }
  
  typedef typename VarArray::value_type VarRef;
  
  VarArray var_array;

  BoolThreeSATConstraintDynamic(StateObj* _stateObj, const VarArray& _var_array) :
	AbstractConstraint(_stateObj), var_array(_var_array)
  { 
	D_ASSERT(var_array.size() == 3);
#ifndef DYNAMICTRIGGERS
    cerr << "This almost certainly isn't going to work... sorry" << endl;
#endif
  }
  
  int dynamic_trigger_count()
  {	
	D_INFO(2,DI_DYSUMCON,"Setting up Dynamic Trigger Constraint for BOOLSATConstraintDynamic");
	return 2;
  }
  
  // Not specialised for 3 sat
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
	  getState(stateObj).setFailed(true);
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
  
  /// Finds the value out of 0,1 and 2 which is not a or b.
  inline int other_val(int a, int b)
  {
	if(a != 0 && b != 0)
	  return 0;
  
	if(a != 1 && b != 1)
	  return 1;
	
	return 2;
  }
  
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
	PROP_INFO_ADDONE(Dyn3SAT);
	int propval = dt->trigger_info();
	//int var_size = var_array.size();
	
	DynamicTrigger* base_dt = dynamic_trigger_start();
	int other_propval;
	
	if(base_dt == dt)
	  other_propval = (base_dt + 1)->trigger_info();
	else
	  other_propval = base_dt->trigger_info();
	
	
    D_INFO(1, DI_DYSUMCON, "Triggering on domain of "+ to_string(propval));

	int unchecked_val = other_val(propval, other_propval);
	
	if(var_array[unchecked_val].inDomain(1))
	{
	  // Found new value to watch
	  dt->trigger_info() = unchecked_val;
	  var_array[unchecked_val].addDynamicTrigger(dt, UpperBound);
	}
	else
	{ var_array[other_propval].propagateAssign(1); }
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array.size());
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
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    for(int i = 0; i < var_array.size(); ++i)
    {
      if(var_array[i].getMax() > 0)
      {
        assignment.push_back(make_pair(i, var_array[i].getMax()));
        return true;
      }
    }
    return false;
  }
};

template<typename VarArray>
AbstractConstraint*
BoolThreeSATConDynamic(StateObj* stateObj, const VarArray& _var_array)
{ return new BoolThreeSATConstraintDynamic<VarArray>(stateObj, _var_array); }
