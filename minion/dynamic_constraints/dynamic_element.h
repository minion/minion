
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

// The triggers in this constraint are set up as follows:
// If the length of the vector is L.

// The first 2 * Dom(Result) literals are, for some j
//   literal 2 * i : attached to assignment i to V[j]
//	 literal 2 * i + 1 : attached to the assignment j in IndexVar 

// After this there are 2 * Dom(Index) literals are, for some j
// literal 2 * i : attached to j in V[i]
// literal 2 * i + 1 : attached to j in Result



// VarToCount = 1 means leq, = 0 means geq.
template<typename VarArray, typename Index, typename Result>
struct ElementConstraintDynamic : public DynamicConstraint
{
  virtual string constraint_name()
  { return "ElementDynamic"; }
  
  //  typedef BoolLessSumConstraintDynamic<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;
  
  // ReversibleInt count;
  VarArray var_array;
  Index indexvar;
  Result resultvar;
  
  int initial_result_dom_min;
  int initial_result_dom_max;
  
  vector<int> current_support;
  
  ElementConstraintDynamic(const VarArray& _var_array, const Index& _index, const Result& _result) :
	var_array(_var_array), indexvar(_index), resultvar(_result)
  { 
	  initial_result_dom_min = resultvar.getInitialMin();
	  initial_result_dom_max = resultvar.getInitialMax();
  }
  
  int dynamic_trigger_count()
  {
	int count = var_array.size() * 2 + 
	(initial_result_dom_max - initial_result_dom_min + 1) * 2 
	+ 1 
	+ 1; 
	current_support.resize(count / 2);           // is int the right type?
	return count;
  }
  
  void find_new_support_for_result(int j)
  {
	int realj = j + initial_result_dom_min;
	
    if(!resultvar.inDomain(realj))
	  return;
	
	int array_size = var_array.size();
    
    // support is value of index
    int oldsupport = max(current_support[j + array_size], indexvar.getMin());  // old support probably just removed
    int maxsupport = indexvar.getMax();
    
	int support = oldsupport;
	
    DynamicTrigger* dt = dynamic_trigger_start();
    while(support <= maxsupport && 
		  !(indexvar.inDomain_noBoundCheck(support) && var_array[support].inDomain(realj)))
      ++support;
    if(support > maxsupport)
    { 
      support = indexvar.getMin();
	  int max_check = min(oldsupport, maxsupport + 1);
      while(support < max_check && 
			!(indexvar.inDomain_noBoundCheck(support) && var_array[support].inDomain(realj)))
        ++support;
      if (support == max_check) 
      {
        D_INFO(2, DI_DYELEMENT, "No support for " + to_string(realj) + " in result");
        resultvar.removeFromDomain(realj); 
        return;
      }
    }
    var_array[support].addDynamicTrigger(dt + 2*j, DomainRemoval, realj);
    indexvar.addDynamicTrigger(dt + 2*j + 1, DomainRemoval, support);
    current_support[j + array_size] = support;
  }
  
  void find_new_support_for_index(int i)
  {
    if(!indexvar.inDomain(i))
	  return;
	
	int resultvarmin = resultvar.getMin();
	int resultvarmax = resultvar.getMax();
	DynamicTrigger* dt = dynamic_trigger_start() + 
	                     (initial_result_dom_max - initial_result_dom_min + 1) * 2;
						 
	if(resultvarmin == resultvarmax)
	{
	  if(!var_array[i].inDomain(resultvarmin))
	    indexvar.removeFromDomain(i);
	  else
	  {
	  	var_array[i].addDynamicTrigger(dt + 2*i, DomainRemoval, resultvarmin);
	    resultvar.addDynamicTrigger(dt + 2*i + 1, DomainRemoval, resultvarmin);
	    current_support[i] = resultvarmin;
	  }
	  return;
	}
	

    // support is value of result
    int oldsupport = max(current_support[i], resultvarmin); // old support probably just removed
    int maxsupport = resultvarmax;
    int support = oldsupport;
	
    //int support = initial_result_dom_min;
	while(support <= maxsupport &&
		  !(resultvar.inDomain_noBoundCheck(support) && var_array[i].inDomain(support)))
	  ++support;
	  
	if(support > maxsupport)
	{ 
	  support = resultvarmin;
	  int max_check = min(oldsupport, maxsupport + 1);
	  while(support < max_check &&     
			!(resultvar.inDomain_noBoundCheck(support) && var_array[i].inDomain(support)))
		++support;
	  if( support >= max_check )      
	  {                                     
	    D_INFO(2, DI_DYELEMENT, "No support for " + to_string(i) + " in index");
	    indexvar.removeFromDomain(i); 
		return;
	  }
	}
	
	var_array[i].addDynamicTrigger(dt + 2*i, DomainRemoval, support);
	resultvar.addDynamicTrigger(dt + 2*i + 1, DomainRemoval, support);
	current_support[i] = support;
  }
  
  void deal_with_assigned_index()
  {
    D_ASSERT(indexvar.isAssigned());
    int indexval = indexvar.getAssignedValue();
    VarRef var = var_array[indexval];
	
    int lower = resultvar.getMin(); 
    if( lower > var.getMin() ) 
    {
      var.setMin(lower);
      ++lower;                      // do not need to check lower bound, we know it's in resultvar
    }
	
    int upper = resultvar.getMax(); 
    if( upper < var.getMax() ) 
    {
      var.setMax(upper);
      --upper;                      // do not need to check upper bound, we know it's in resultvar
    }
    
    for(int i = lower; i <= upper; ++i)
    {
      if(!(resultvar.inDomain(i)))
        var.removeFromDomain(i); 
    }
  }
  
  virtual void full_propogate()
  {
	D_INFO(2, DI_DYELEMENT, "Setup Triggers");
	int array_size = var_array.size(); 
	int result_dom_size = initial_result_dom_max - initial_result_dom_min + 1;
	
	// Setup SupportLostForIndexValue(i,j)
	// Here we are supporting values in the index variable
	// So for each variable in the index variable, we want to ensure
	
	// Couple of quick sanity-propagations.
	// We define UNDEF = false ;)
	indexvar.setMin(0);
	indexvar.setMax(array_size - 1);
	for(int i = 0; i < array_size; ++i)
	{
	  current_support[i] = initial_result_dom_min-1;        // will be incremented if support sought
	  if(indexvar.inDomain(i))
	    find_new_support_for_index(i);
	}
	
	for(int i = 0; i < result_dom_size; ++i)
	{
	  current_support[i+array_size] = -1;   // will be incremented if support sought
	  if(resultvar.inDomain(i + initial_result_dom_min))
	    find_new_support_for_result(i);
	}
	
	DynamicTrigger* dt = dynamic_trigger_start();
	
	dt += var_array.size() * 2 +
	  (initial_result_dom_max - initial_result_dom_min + 1) * 2;
	
	// for(int i = initial_result_dom_min; i <= initial_result_dom_max; ++i)
	// {
	// resultvar.addDynamicTrigger(dt, DomainRemoval, i);
	// ++dt;
	// }
	resultvar.addDynamicTrigger(dt, DomainChanged);
	++dt;
	
	indexvar.addDynamicTrigger(dt, Assigned);
	
  }
  
  
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* trig)
  {
    D_INFO(2, DI_DYELEMENT, "Start Propagation");
	DynamicTrigger* dt = dynamic_trigger_start();
	unsigned pos = trig - dt;
	unsigned array_size = var_array.size();
	unsigned result_support_triggers = (initial_result_dom_max - initial_result_dom_min + 1) * 2;
	unsigned index_support_triggers =  array_size * 2;
	// int when_index_assigned_triggers = (initial_result_dom_max - initial_result_dom_min + 1);
	if(pos < result_support_triggers)
	{// It was a value in the result var which lost support
	  D_INFO(2, DI_DYELEMENT, "Find new support for result var assigned " + to_string(pos/2));
	  find_new_support_for_result(pos / 2);
	  return;
	}
	pos -= result_support_triggers;
	
	if(pos < index_support_triggers)
	{// A value in the index var lost support
	  D_INFO(2, DI_DYELEMENT, "Find new support for index var assigned " + to_string(pos/2));
	  find_new_support_for_index( pos / 2 );
	  return;
	}
	pos -= index_support_triggers;
	
	// if(pos < when_index_assigned_triggers)
	if (pos == 0)
	{ // A value was removed from result var
	  if(indexvar.isAssigned())
	  {
		deal_with_assigned_index();
	    //D_ASSERT(!resultvar.inDomain(pos + initial_result_dom_min));
		// D_INFO(2, DI_DYELEMENT, "indexvar assigned, so must remove " + to_string(pos + initial_result_dom_min) + " from var " + to_string(indexvar.getAssignedValue()));
	    //var_array[indexvar.getAssignedValue()].removeFromDomain(pos + initial_result_dom_min);
	  }
	  return;
	}
	
	D_ASSERT(pos == 1);
    // index has become assigned.
	
	D_INFO(2, DI_DYELEMENT, "Index var assigned " + to_string(indexvar.getAssignedValue()));
	
	deal_with_assigned_index();
  }
  
    virtual BOOL check_assignment(vector<int> v)
	{
	  D_ASSERT(v.size() == var_array.size() + 2);
	  int resultvariable = v[v.size() - 1];
	  int indexvariable = v[v.size() - 2];
	  if(indexvariable < 0 || indexvariable >= (int)v.size() - 2)
	    return false;
	  return v[indexvariable] == resultvariable;
	}
	
    virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
	vars.reserve(var_array.size() + 2);
    for(unsigned i = 0; i < var_array.size(); ++i)
	  vars.push_back(var_array[i]);
	vars.push_back(indexvar);
	vars.push_back(resultvar);
	return vars;
  }
  
};


// Note: we pass into the first vector into this function by value rather
// than by const reference because we want to change it.
template<typename VarRef1, typename VarRef2>
DynamicConstraint*
DynamicElementCon(vector<VarRef1> vararray, const vector<VarRef2>& v1)
{ 
  // Because we can only have two things which are parsed at the moment, we do
  // a dodgy hack and store the last variable on the end of the vararray
  // during parsing. Now we must pop it back off.
  VarRef1 assignval = vararray.back();
  vararray.pop_back();
  return new ElementConstraintDynamic<vector<VarRef1>, VarRef2, VarRef1>(vararray, v1[0], assignval);  
}

BUILD_DYNAMIC_CONSTRAINT2(CT_WATCHED_ELEMENT, DynamicElementCon);



