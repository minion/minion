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

/** @help constraints;element Description
The constraint 

   element(vec, i, e)

specifies that, in any solution, vec[i] = e.
*/

/** @help constraints;element Reifiability
This constraint is not reifiable.
*/

/** @help constraints;element Notes 

Warning: This constraint is not confluent. Depending on the order the
propagators are called in Minion, the number of search nodes may vary when
using element. To avoid this problem, use watchelement instead. More details
below.

The level of propagation enforced by this constraint is not named, however it
works as follows. For constraint vec[i]=e:

- After i is assigned, ensures that min(vec[i]) = min(e) and 
  max(vec[i]) = max(e).

- When e is assigned, removes idx from the domain of i whenever e is not an
  element of the domain of vec[idx].

- When m[idx] is assigned, removes idx from i when m[idx] is not in the domain
  of e.

This level of consistency is designed to avoid the propagator having to scan
through vec, except when e is assigned. It does a quantity of cheap propagation
and may work well in practise on certain problems.

Element is not confluent, which may cause the number of search nodes to vary
depending on the order in which constraints are listed in the input file, or 
the order they are called in Minion. For example, the following input causes
Minion to search 41 nodes.

MINION 3
**VARIABLES**
DISCRETE x[5] {1..5}
**CONSTRAINTS**
element([x[0],x[1],x[2]], x[3], x[4])
alldiff([x])
**EOF**

However if the two constraints are swapped over, Minion explores 29 nodes.
As a rule of thumb, to get a lower node count, move element constraints
to the end of the list.
*/

/** @help constraints;element References 
See the entry 

   constraints watchelement

for details of an identical constraint that enforces generalised arc
consistency.
*/

template<typename VarArray, typename IndexRef>
struct ElementConstraint : public Constraint
{
  virtual string constraint_name()
  { return "Element"; }
  
  typedef typename VarArray::value_type VarRef;
  VarArray var_array;
  IndexRef index_ref;
  VarRef result_var;
  ElementConstraint(StateObj* _stateObj, const VarArray& _var_array, const IndexRef& _index_ref, const VarRef& _result_var) :
    Constraint(_stateObj), var_array(_var_array), index_ref(_index_ref), result_var(_result_var)
  { }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
	  t.push_back(make_trigger(var_array[i], Trigger(this, i), Assigned));
	
	t.push_back(make_trigger(index_ref, Trigger(this, -1), Assigned));
	t.push_back(make_trigger(result_var, Trigger(this, -2), Assigned));
    return t;
  }
  
  //  virtual Constraint* reverse_constraint()
  
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
  {
	PROP_INFO_ADDONE(NonGACElement);
    if(index_ref.isAssigned())
    {
      int index = checked_cast<int>(index_ref.getAssignedValue());
	  if(index < 0 || index >= (int)var_array.size())
	  {
	    getState(stateObj).setFailed(true);
		return;
	  }
	  DomainInt val_min = max(result_var.getMin(), var_array[index].getMin());
	  DomainInt val_max = min(result_var.getMax(), var_array[index].getMax());
	  result_var.setMin(val_min);
	  var_array[index].setMin(val_min);
	  result_var.setMax(val_max);
	  var_array[index].setMax(val_max);
    }
    else
    {
      if(prop_val>=0)
      {
		DomainInt assigned_val = var_array[prop_val].getAssignedValue();
		if(index_ref.inDomain(prop_val) && !result_var.inDomain(assigned_val))  //perhaps the check if prop_val is indomain of index_ref is not necessary.
        {
            if(index_ref.isBound())
            {
                if(prop_val==index_ref.getMax()) index_ref.setMax(prop_val-1);
                if(prop_val==index_ref.getMin()) index_ref.setMin(prop_val+1);
            }
            else
            {
                index_ref.removeFromDomain(prop_val);
            }
        }
        
      }
      else
      {
		D_ASSERT(prop_val == -2);
		DomainInt assigned_val = result_var.getAssignedValue();
		int array_size = var_array.size();
		for(int i = 0; i < array_size; ++i)
		{
		  if(index_ref.inDomain(i) && !var_array[i].inDomain(assigned_val)) // fixed here.
          {
              if(index_ref.isBound())
                {
                    if(i==index_ref.getMax()) index_ref.setMax(i-1);
                    if(i==index_ref.getMin()) index_ref.setMin(i+1);
                }
                else
                {
                    index_ref.removeFromDomain(i);
                }
          }
		}
      }
    }
  }

  virtual void full_propagate()
  {
    if(index_ref.isAssigned())
    {
      int index = checked_cast<int>(index_ref.getAssignedValue());
	  if(index < 0 || index >= (int)var_array.size())
	  {
	    getState(stateObj).setFailed(true);
		return;
	  }
      DomainInt val_min = max(result_var.getMin(), var_array[index].getMin());
      DomainInt val_max = min(result_var.getMax(), var_array[index].getMax());
      result_var.setMin(val_min);
      var_array[index].setMin(val_min);
      result_var.setMax(val_max);
      var_array[index].setMax(val_max);
    }
    
    int array_size = var_array.size();
	// Constrain the index variable to have only indices in range.
    if(index_ref.getMin()<0)
    {
        index_ref.setMin(0);
    }
    if(index_ref.getMax()>=array_size)
    {
        index_ref.setMax(array_size-1);
    }
    if(getState(stateObj).isFailed()) return;
    
    // Should use the new iterators here. Check each value of result_var to see 
    // if it's in one of var_array. 
    // Only done at root, so who cares that it takes a while?
    if(!result_var.isBound())
    {
        for(DomainInt i=result_var.getMin(); i<=result_var.getMax(); i++)
        {
            if(result_var.inDomain(i))
            {
                BOOL supported=false;
                for(DomainInt j=index_ref.getMin(); j<=index_ref.getMax(); j++)
                {
                    if(var_array[j].inDomain(i))
                    {
                        supported=true;
                        break;
                    }
                }
                if(!supported)
                {
                    result_var.removeFromDomain(i);
                }
            }
        }
    }
    else
    {// result_var is a bound variable
        // iterate up from the minimum
        while(!getState(stateObj).isFailed())
        {
            DomainInt i=result_var.getMin();
            BOOL supported=false;
            for(DomainInt j=index_ref.getMin(); j<=index_ref.getMax(); j++)
            {
                if(var_array[j].inDomain(i))
                {
                    supported=true;
                    break;
                }
            }
            if(!supported)
            {
                result_var.setMin(i+1);
            }
            else
                break;
        }
        // now iterate down from the top.
        while(!getState(stateObj).isFailed())
        {
            DomainInt i=result_var.getMax();
            BOOL supported=false;
            for(DomainInt j=index_ref.getMin(); j<=index_ref.getMax(); j++)
            {
                if(var_array[j].inDomain(i))
                {
                    supported=true;
                    break;
                }
            }
            if(!supported)
            {
                result_var.setMax(i-1);
            }
            else
                break;
        }
    }
    
    if(getState(stateObj).isFailed()) return;
    
    for(int i = index_ref.getMin();i <= index_ref.getMax(); i++)
	{
      if(index_ref.inDomain(i) && var_array[i].isAssigned())
      {
        DomainInt assigned_val = var_array[i].getAssignedValue();
        if(!result_var.inDomain(assigned_val))
        {
            if(index_ref.isBound())
            {
                if(i==index_ref.getMax()) index_ref.setMax(i-1);
                if(i==index_ref.getMin()) index_ref.setMin(i+1);
            }
            else
            {
                
                index_ref.removeFromDomain(i);
            }
        }
      }
	}
    
    if(result_var.isAssigned())
    {
      DomainInt assigned_val = result_var.getAssignedValue();
      for(int i = 0; i < array_size; ++i)
      {
        if(index_ref.inDomain(i) && !var_array[i].inDomain(assigned_val))  // fixed here.
        {
            if(index_ref.isBound())
            {
                if(i==index_ref.getMax()) index_ref.setMax(i-1);
                if(i==index_ref.getMin()) index_ref.setMin(i+1);
            }
            else
            {
                index_ref.removeFromDomain(i);
            }
        }
      }
    }
    
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    int length = v_size;
    if(v[length-2] < 0 || v[length-2] > length - 3)
	  return false;
    return v[checked_cast<int>(v[length-2])] == v[length-1];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> array;
	array.reserve(var_array.size() + 2);
	for(unsigned int i=0;i<var_array.size(); ++i)
	  array.push_back(var_array[i]);
    array.push_back(index_ref);
    array.push_back(result_var);
    return array;
  }
};

// Note: we pass into the first vector into this function by value rather
// than by const reference because we want to change it.
template<typename Var1, typename Var2>
Constraint*
ElementCon(StateObj* stateObj, Var1 vararray, const Var2& v1)
{ 
  // Because we can only have two things which are parsed at the moment, we do
  // a dodgy hack and store the last variable on the end of the vararray
  // during parsing. Now we must pop it back off.
  typedef typename Var1::value_type VarRef1;
  typedef typename Var2::value_type VarRef2;
  VarRef1 assignval = vararray.back();
  vararray.pop_back();
  return new ElementConstraint<Var1, VarRef2>(stateObj, vararray, v1[0], assignval);  
}

BUILD_CONSTRAINT2(CT_ELEMENT, ElementCon);

