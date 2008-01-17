/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/


/*
 Chris, note there is a bit where I comment out how I really want to propagate instead
 of calling limit_reached, but I couldn't remember if you told me I could have
 that method or not.    It saves going through the whole clause. 
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
template<typename VarArray, typename VarSum, int VarToCount = 1 >
struct BoolLessSumConstraintDynamic : public DynamicConstraint
{
  virtual string constraint_name()
  { if(VarToCount) return "Bool<=SumDynamic"; else return "Bool>=SumDynamic"; }
  
  typedef BoolLessSumConstraintDynamic<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;
  
  // ReversibleInt count;
  VarArray var_array;
#ifdef WATCHEDLITERALS
  MemOffset unwatched_indexes;
  int last;
  int num_unwatched;
#else
/// XXX : These first two variables are just here to make the code compile.
/// This code has basically decayed beyound the point of working.. :(
  int num_unwatched;
  int last;

  BackTrackOffset vals_watched;
  BackTrackOffset unwatched_indexes;     // no promise this will work in this case
  //ReversibleInt last;
  BOOL& values_watched(int i)
  { return static_cast<BOOL*>(vals_watched.get_ptr())[i]; }
#endif


  int& unwatched(int i)
  { return static_cast<int*>(unwatched_indexes.get_ptr())[i]; }
  
  VarSum var_sum;
  
  BoolLessSumConstraintDynamic(const VarArray& _var_array, VarSum _var_sum) :
	var_array(_var_array), var_sum(_var_sum)
  { 
    D_ASSERT((VarToCount == 0) || (VarToCount == 1));
#ifndef WATCHEDLITERALS
    cerr << "This almost certainly isn't going to work... sorry" << endl;
#endif
  }
  
  int dynamic_trigger_count()
  {
	
	
	// Sum of 1's is >= K
	// == Number of 1's is >=K         // this is the one I want to do
	// == Number of 0's is <= N-K
	
	D_INFO(2,DI_DYSUMCON,"Setting up Dynamic Trigger Constraint for BoolLessSumConstraintDynamic");
	
	int array_size = var_array.size();

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
        else
        {
          
#ifndef WATCHEDLITERALS
          // Note that this BOOL array could be compacted using masks for lookup
          vals_watched.request_bytes(sizeof(BOOL) * array_size);
          for(int i = 0; i < array_size; i++) 
            values_watched(i) = false;
#endif

          num_unwatched = array_size - var_sum - 1 ;
          D_ASSERT(num_unwatched >= 0);

          unwatched_indexes.request_bytes(sizeof(unsigned) * num_unwatched);
          // above line might request 0 bytes
          last = 0;

          return var_sum + 1;
        }
  }
  
  /*
  int occ_count()
  {
	if (VarToCount)
	  return var_sum;
	else
	  return var_array.size() - var_sum;
  }
  */
  
  virtual void full_propagate()
  {
	DynamicTrigger* dt = dynamic_trigger_start();
	
	int array_size = var_array.size(); 
	int triggers_wanted = var_sum + 1;
        int index;
	
	for(index = 0; (index < array_size) && (triggers_wanted > 0); ++index) 
	{
	  if(var_array[index].inDomain(1 - VarToCount))              
	  {
		// delay setting up triggers in case we don't need to
		--triggers_wanted;
	  }
	}       

       

	D_ASSERT(triggers_wanted >= 0);
	
	if(triggers_wanted > 1)    // Then we have failed, forget it.
	{
	  Controller::fail();
	  return;
	}
	else if(triggers_wanted == 1)      // Then we can propagate 
	{                               // We never even set up triggers
          for(int i = 0; i < array_size; ++i)
          {
            if(var_array[i].inDomain(1 - VarToCount))
            {
              D_INFO(1, DI_DYSUMCON, to_string(i) + " watched, so pruning.");
              if(VarToCount)
                    var_array[i].setMax(0);
                  else
                    var_array[i].setMin(1);          
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
            if(var_array[i].inDomain(1 - VarToCount))
            {
              dt->trigger_info() = i;
              var_array[i].addDynamicTrigger(dt, VarToCount ? LowerBound : UpperBound);
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
          
          for(int i=index; i < array_size; ++i)
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
  {
#ifndef WATCHEDLITERALS 
    DynamicTrigger* start = dynamic_trigger_start();
    DynamicTrigger* end = start + var_sum + 1;	
	for(DynamicTrigger* ptr = start; ptr < end; ptr++)
	{  D_ASSERT(values_watched(ptr->trigger_info())); }
	
	int array_size = var_array.size(); 
	for(int i = 0; i < array_size; ++i)
	{
          if(values_watched(i))
	  {
		DynamicTrigger* ptr = start;
		while(ptr < end && ptr->trigger_info() != i)
		  ptr++;
		D_ASSERT(ptr != end);
	  }
	}
#endif
	
	return true;
  }
  
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
	PROP_INFO_ADDONE(DynSum);
    D_ASSERT(check_consistency());
	int propval = dt->trigger_info();
    D_INFO(1, DI_DYSUMCON, "Triggering on domain of "+ to_string(propval));
#ifndef WATCHEDLITERALS 
	D_ASSERT(values_watched(propval));
#endif
	D_ASSERT(var_array[propval].getAssignedValue() == VarToCount);
    // VarIterator<VarRef> it(var_array[i], propval + 1);
	// should generalise
	// and will need to loop round for watched lits	

	bool found_new_support = false;
        
#ifdef WATCHEDLITERALS

        int loop;
        int j;

        for(loop = 0 ; (!found_new_support) && loop < num_unwatched ; ++loop )
        {
          D_ASSERT(num_unwatched > 0);

          j = (last+1+loop) % num_unwatched; 
          if(var_array[unwatched(j)].inDomain(1 - VarToCount))
          {
            found_new_support = true;
          }
        }

	if (found_new_support)         // so we have found a new literal to watch
	{
      int& unwatched_index = unwatched(j);

	  dt->trigger_info() = unwatched_index;
	  var_array[unwatched_index].addDynamicTrigger(
              dt, 
              VarToCount ? LowerBound : UpperBound); 

	  unwatched_index = propval;       
	  last = j;

	  return;
	}

#else
	int array_size = var_array.size();
	int i = propval+1;

	while((i < array_size) &&
		  (values_watched(i) || !var_array[i].inDomain(1 - VarToCount)))
	{ ++i; }
	if(i < array_size)
	  found_new_support = true;
	
	if (found_new_support)         // so we have found a new literal to watch
	{
	  values_watched(propval) = false;
	  D_ASSERT(!values_watched(i) && var_array[i].inDomain(1 - VarToCount));    
	  dt->trigger_info() = i;
	  var_array[i].addDynamicTrigger(dt, VarToCount ? LowerBound : UpperBound); 

	  values_watched(i) = true;
	  return;
	}
#endif

	D_INFO(1,DI_DYSUMCON,"Limit Reached in WL sum (0/1) constraint");	
	// there is no literal to watch, we need to propagate
	
	DynamicTrigger* dt2 = dynamic_trigger_start();
	
	for(int z = 0; z < var_sum + 1; ++z)
	{
	  if(dt != dt2)       // that one has just been set the other way
	  {
	    if(VarToCount)
		  var_array[dt2->trigger_info()].setMax(0);
		else
		  var_array[dt2->trigger_info()].setMin(1);
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
	  count += (v[i] != VarToCount);
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

template<typename VarArray,  typename VarSum>
DynamicConstraint*
BoolLessEqualSumConDynamic(const VarArray& _var_array,  VarSum _var_sum)
{ 
  return new BoolLessSumConstraintDynamic<VarArray,VarSum>(_var_array,
		  runtime_val(_var_array.size() - _var_sum)); 
}

template<typename VarArray,  typename VarSum>
DynamicConstraint*
BoolGreaterEqualSumConDynamic(const VarArray& _var_array,  VarSum _var_sum)
{ 
  return new BoolLessSumConstraintDynamic<VarArray,VarSum,0>(_var_array, _var_sum); 
}


#include "dynamic_sum_sat.h"
#include "dynamic_binary_sat.h"
#include "dynamic_3_sat.h"

inline DynamicConstraint*
BuildCT_WATCHED_LEQSUM(const light_vector<BoolVarRef>& t1, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b)
{ 
  if(reify) 
  { 
    cerr << "Cannot reify 'watched literal' constraints. Sorry." << endl; 
	  exit(0); 
  } 
  else 
  { return BoolLessEqualSumConDynamic(t1, runtime_val(b.vars[1][0].pos)); } \
}

template<typename T>
DynamicConstraint*
BuildCT_WATCHED_LEQSUM(const T& t1, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b)
{ 
  cerr << "Watched LeqSum only works on Boolean variables at present. Sorry!" << endl;
  exit(1);
}

inline DynamicConstraint*
BuildCT_WATCHED_GEQSUM(const light_vector<BoolVarRef>& t1, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b)
{ 
  if(reify) 
  { 
    cerr << "Cannot reify 'watched literal' constraints. Sorry." << endl; 
	exit(0); 
  } 
  else 
  {
	int sum = b.vars[1][0].pos;
#ifndef SATSPECIAL1
	if(sum == 1)
	{
#ifndef SATSPECIAL2
	  if(t1.size() == 2)
	  {
		return BoolBinarySATConDynamic(t1);
	  }
#ifndef SATSPECIAL3
	  else if(t1.size() == 3)
	  {
		return BoolThreeSATConDynamic(t1);
	  }
#endif
	  else
#endif
	  {
	    return BoolSATConDynamic(t1);
	  }
	}
	else
#endif
	{
	  return BoolGreaterEqualSumConDynamic(t1, runtime_val(sum)); 
	}
  }
}

template<typename T>
DynamicConstraint*
BuildCT_WATCHED_GEQSUM(const T& t1, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b)
{ 
  cerr << "Watched GeqSum only works on Boolean variables at present. Sorry!" << endl;
  exit(1);
}
