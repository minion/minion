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

/** @help constraints;litsumgeq Description 
The constraint litsumgeq(vec1, vec2, c) ensures that there exists at least c
distinct indices i such that vec1[i] = vec2[i].
*/

/** @help constraints;litsumgeq Notes
A SAT clause {x,y,z} can be created using:

   litsumgeq([x,y,z],[1,1,1],1)

Note also that this constraint is more efficient for smaller values of c. For
large values consider using watchsumleq.
*/

/** @help constraints;litsumgeq Reifiability
This constraint is NOT reifiable.
*/

/** @help constraints;litsumgeq References
See also

   help constraints watchsumleq
   help constraints watchsumgeq
*/


template<typename VarArray, typename ValueArray, typename VarSum>
struct LiteralSumConstraintDynamic : public DynamicConstraint
{
  virtual string constraint_name()
  { return "LiteralSumDynamic"; }
  
 typedef typename VarArray::value_type VarRef;
  
  VarArray var_array;
  ValueArray value_array;
  MemOffset unwatched_indexes;
  int last;
  int num_unwatched;
    
  int& unwatched(int i)
  { return static_cast<int*>(unwatched_indexes.get_ptr())[i]; }
  
  VarSum var_sum;
  
  LiteralSumConstraintDynamic(StateObj* _stateObj,const VarArray& _var_array, ValueArray _val_array, VarSum _var_sum) :
	DynamicConstraint(_stateObj), var_array(_var_array), value_array(_val_array), var_sum(_var_sum)
  { }
  
  int dynamic_trigger_count()
  {
	D_INFO(2,DI_DYSUMCON,"Setting up Dynamic Trigger Constraint for LiteralSumConstraintDynamic");
	
	int array_size = var_array.size();
	
	// XXX comment out this optimisation for now
	/*
	if (var_sum == array_size)
	{
	  // In this case every var will be set to 1
	  // This will happen before triggers set up in full_propagate
	  // Thus zero triggers are needed
	  // However we will say that 1 is needed 
	  //     because I don't know if setup code will work when 0 triggers requested
	  //     Should set to 0 and test it.
	  return 1;
	}
	else*/
	{
	  
	  num_unwatched = array_size - var_sum - 1 ;
	  D_ASSERT(num_unwatched >= 0);
	  
	  unwatched_indexes = getMemory(stateObj).nonBackTrack().request_bytes(sizeof(unsigned) * num_unwatched);
	  // above line might request 0 bytes
	  last = 0;
	  
	  return var_sum + 1;
	}
  }
    
  virtual void full_propagate()
  {
	DynamicTrigger* dt = dynamic_trigger_start();
	
	int array_size = var_array.size(); 
	int triggers_wanted = var_sum + 1;
	int index;
	
	for(index = 0; (index < array_size) && (triggers_wanted > 0); ++index) 
	{
	  if(var_array[index].inDomain(value_array[index]))              
	  {
		// delay setting up triggers in case we don't need to
		--triggers_wanted;
	  }
	}       
	
	D_ASSERT(triggers_wanted >= 0);
	
	if(triggers_wanted > 1)    // Then we have failed, forget it.
	{
	  getState(stateObj).setFailed(true);
	  return;
	}
	else if(triggers_wanted == 1)      // Then we can propagate 
	{                               // We never even set up triggers
	  for(int i = 0; i < array_size; ++i)
	  {
		if(var_array[i].inDomain(value_array[i]))
		{
		  D_INFO(1, DI_DYSUMCON, to_string(i) + " watched, so pruning.");
		  var_array[i].propagateAssign(value_array[i]);
		}
	  }
	}
	else                                // Now set up triggers
	{
	  D_ASSERT(triggers_wanted == 0);
	  
	  int j = 0;
	  
	  // We only look at the elements of vararray that we looked at before
	  // Exactly triggers_wanted of them have the val in their domain.
	  
	  for(int i = 0; (i < index); ++i)   // remember index was the elts we looked at
	  { 
		if(var_array[i].inDomain(value_array[i]))
		{
		  dt->trigger_info() = i;
		  var_array[i].addDynamicTrigger(dt, DomainRemoval, value_array[i]);
		  ++dt;
		}
		else
		{
		  unwatched(j) = i;
		  ++j;
		  // When run at root we could optimise as follows
		  //    If VarToCount not in domain then do not put j in unwatched
		  //      Instead decrement num_unwatched
		}
	  }
	  
	  for(int i = index; i < array_size; ++i)
	  {
		unwatched(j) = i;
		++j;
	  }
	  
	  D_ASSERT(j == num_unwatched);
	}
	return;
  }
  
  /// Checks the consistency of the constraint's data structures
  //
  //  NOT updated for new type of watched data structure
  //  
  BOOL check_consistency()
  {	return true; }
  
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
	PROP_INFO_ADDONE(DynLitWatch);
    D_ASSERT(check_consistency());
	int propval = dt->trigger_info();
    D_INFO(1, DI_DYSUMCON, "Triggering on domain of "+ to_string(propval));

	D_ASSERT(!var_array[propval].inDomain(value_array[propval]));
	
	BOOL found_new_support = false;
	
	int j = 0;
	
	for(int loop = 0 ; (!found_new_support) && loop < num_unwatched ; )
	{
	  D_ASSERT(num_unwatched > 0);
	  
	  j = (last+1+loop) % num_unwatched; 
	  if(var_array[unwatched(j)].inDomain(value_array[unwatched(j)]))
	  {
		found_new_support = true;
	  }
	  // XXX What is going on here? check in dynamic_sum!
	  {
		++loop;
	  } 
	}
	
	if (found_new_support)         // so we have found a new literal to watch
	{
	  int& unwatched_index = unwatched(j);
	  
	  // propval gives array index of old watched lit
	  
	  dt->trigger_info() = unwatched_index;
	  var_array[unwatched_index].addDynamicTrigger(dt,DomainRemoval,
												   value_array[unwatched_index]); 
	  
	  unwatched_index = propval;       
	  last = j;
	  return;
	}
	
	
	D_INFO(1,DI_DYSUMCON,"Limit Reached in WL sum (0/1) constraint");	
	// there is no literal to watch, we need to propagate
	
	DynamicTrigger* dt2 = dynamic_trigger_start();
	
	for(int z = 0; z < var_sum + 1; ++z)
	{
	  if(dt != dt2)       // that one has just been set the other way
	  {
		var_array[dt2->trigger_info()].propagateAssign(value_array[dt2->trigger_info()]);
	  }
	  dt2++;
	}
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    D_ASSERT(v.size() == var_array.size());
    int v_size = v.size();
	int count = 0;
	for(int i = 0; i < v_size; ++i)
	  count += (v[i] == value_array[i]);
	return count >= var_sum;
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

template<typename VarArray,  typename ValArray, typename VarSum>
DynamicConstraint*
LiteralSumConDynamic(StateObj* stateObj,const VarArray& _var_array,  const ValArray& _val_array, VarSum _var_sum)
{ return new LiteralSumConstraintDynamic<VarArray,ValArray,VarSum>(stateObj, _var_array, _val_array, _var_sum); }

template<typename T1>
DynamicConstraint* 
BuildCT_WATCHED_LITSUM(StateObj* stateObj,const T1& t1, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b)
{
  if(reify)
  {
    cerr << "Cannot reify 'watched literal' constraints. Sorry." << endl; 
	  exit(0);
  }
  else
  { 
	vector<int> values;
	for(unsigned i = 0; i < b.vars[1].size(); ++i)
	  values.push_back(b.vars[1][i].pos);
	return LiteralSumConDynamic(stateObj, t1, values, runtime_val(b.vars[2][0].pos)); 
  }
}



