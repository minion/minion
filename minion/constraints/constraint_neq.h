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


/** @help constraints;alldiff Description
Forces the input vector of variables to take distinct values.
*/

/** @help constraints;alldiff Example 
Suppose the input file had the following vector of variables defined:

DISCRETE myVec[9] {1..9}

To ensure that each variable takes a different value include the
following constraint:

alldiff(myVec)
*/

/** @help constraints;alldiff Notes
Enforces the same level of consistency as a clique of not equals 
constraints.
*/

/** @help constraints;alldiff References
See

   help constraints gacalldiff

for the same constraint that enforces GAC.
*/

#ifndef CONSTRAINT_NEQ_H
#define CONSTRAINT_NEQ_H

template<typename VarArray>
struct NeqConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Neq"; }
  
  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;
  
  VarArray var_array;
  
  NeqConstraint(StateObj* _stateObj, const VarArray& _var_array) : AbstractConstraint(_stateObj),
    var_array(_var_array)
  { }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
	  t.push_back(make_trigger(var_array[i], Trigger(this, i), Assigned));
    return t;
  }
  
  virtual AbstractConstraint* reverse_constraint()
  { return new CheckAssignConstraint<VarArray, NeqConstraint>(stateObj, var_array, *this); }
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
  {
	PROP_INFO_ADDONE(ArrayNeq);
    DomainInt remove_val = var_array[prop_val].getAssignedValue();
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
    {
      if(i != prop_val)
	  {
		if(var_array[i].isBound())
	    {
		  if(var_array[i].getMin() == remove_val)
		    var_array[i].setMin(remove_val + 1);
		  if(var_array[i].getMax() == remove_val)
		    var_array[i].setMax(remove_val - 1);
	    }
		else
		  var_array[i].removeFromDomain(remove_val);
	  }
    }
	
  }
  
  virtual BOOL full_check_unsat()
  { 
    int v_size = var_array.size();
    for(int i = 0; i < v_size; ++i)
	{
	  if(var_array[i].isAssigned())
	  {
	  
	    for(int j = i + 1; j < v_size; ++j)
		{
		  if(var_array[j].isAssigned())
		  {
		    if(var_array[i].getAssignedValue() == var_array[j].getAssignedValue())
		      return true;
		  }
		}
		
	  }
	}
	
	return false;
  }
  
  virtual BOOL check_unsat(int i, DomainDelta)
  {
    int v_size = var_array.size();
	D_ASSERT(var_array[i].isAssigned());
	DomainInt assign_val = var_array[i].getAssignedValue();
    for(int loop = 0; loop < v_size; ++loop)
	{
	  if(loop != i)
	  {
	    if(var_array[loop].isAssigned() && 
		   var_array[loop].getAssignedValue() == assign_val)
		return true;
	  }
	}
	return false;
  }
  
  
  virtual void full_propagate()
  {
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
      if(var_array[i].isAssigned())
      {
		DomainInt remove_val = var_array[i].getAssignedValue();
		for(int j = 0; j < array_size;++j)
		{
		  if(i != j)
		  {
			if(var_array[j].isBound())
			{
			  if(var_array[j].getMin() == remove_val)
				var_array[j].setMin(remove_val + 1);
			  if(var_array[j].getMax() == remove_val)
				var_array[j].setMax(remove_val - 1);
			}
			else
			  var_array[j].removeFromDomain(remove_val);
		  }
		}
      }
  }
	
	virtual BOOL check_assignment(DomainInt* v, int v_size)
	{
	  D_ASSERT(v_size == var_array.size());
	  int array_size = v_size;
	  for(int i=0;i<array_size;i++)
		for( int j=i+1;j<array_size;j++)
		  if(v[i]==v[j]) return false;
	  return true;
	}
	
	virtual vector<AnyVarRef> get_vars()
	{
	  vector<AnyVarRef> vars;
	  vars.reserve(var_array.size());
	  for(unsigned i = 0; i < var_array.size(); ++i)
	    vars.push_back(var_array[i]);
	  return vars;
	}
	
	
	 // Getting a satisfying assignment here is too hard, we don't want to have to
	 // build a matching.
	 virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
   {
     MAKE_STACK_BOX(c, DomainInt, var_array.size());
     
     for(int i = 0; i < var_array.size(); ++i)
     {
       if(!var_array[i].isAssigned()) 
       {  
         assignment.push_back(make_pair(i, var_array[i].getMin()));
         assignment.push_back(make_pair(i, var_array[i].getMax()));
         return true;
       }
       else
         c.push_back(var_array[i].getAssignedValue());
     }
    
    if(check_assignment(c.begin(), c.size()))
    {  // Put the complete assignment in the box.
      for(int i = 0; i < var_array.size(); ++i)
        assignment.push_back(make_pair(i, c[i])); 
      return true;
    }
    return false;
   }
   
   
  };

template<typename VarArray>
AbstractConstraint*
BuildCT_ALLDIFF(StateObj* stateObj, const VarArray& var_array, ConstraintBlob&)
{ return new NeqConstraint<VarArray>(stateObj, var_array); }

#endif
