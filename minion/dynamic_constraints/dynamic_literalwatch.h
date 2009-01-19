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
This constraint is reifyimply'able but not reifiable.
*/

/** @help constraints;litsumgeq References
See also

   help constraints watchsumleq
   help constraints watchsumgeq
*/

#ifndef CONSTRAINT_DYNAMIC_LITWATCH_H
#define CONSTRAINT_DYNAMIC_LITWATCH_H

template<typename VarArray, typename ValueArray, typename VarSum>
struct LiteralSumConstraintDynamic : public AbstractConstraint
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
	AbstractConstraint(_stateObj), var_array(_var_array), value_array(_val_array), var_sum(_var_sum)
  {   
  	int array_size = var_array.size();
	  
	  num_unwatched = array_size - var_sum - 1 ;
	  D_ASSERT(num_unwatched >= 0);
	  
	  unwatched_indexes = getMemory(stateObj).nonBackTrack().request_bytes(sizeof(unsigned) * num_unwatched);
	  // above line might request 0 bytes
	  last = 0;
  }
  
  int dynamic_trigger_count()
  {
    if(var_sum < 0)
      return 0;
    if(var_sum > var_array.size())
      return var_array.size() + 1;
    return var_sum + 1;
  }
    
  virtual void full_propagate()
  {
    if(var_sum <= 0)
      return;
      
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
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array.size());
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
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    for(int i = 0; i < var_array.size(); ++i)
    {
      if(var_array[i].inDomain(value_array[i]))
      {
        assignment.push_back(make_pair(i, value_array[i]));
        return true;
      }
    }
    return false;
  }
  
};

template<typename VarArray,  typename ValArray, typename VarSum>
AbstractConstraint*
LiteralSumConDynamic(StateObj* stateObj,const VarArray& _var_array,  const ValArray& _val_array, VarSum _var_sum)
{ return new LiteralSumConstraintDynamic<VarArray,ValArray,VarSum>(stateObj, _var_array, _val_array, _var_sum); }

template<typename T1>
AbstractConstraint* 
BuildCT_WATCHED_LITSUM(StateObj* stateObj,const T1& t1, ConstraintBlob& b)
{
  D_ASSERT(b.constants[1].size());
  return LiteralSumConDynamic(stateObj, t1, b.constants[0], runtime_val(b.constants[1][0])); 
}

#endif
