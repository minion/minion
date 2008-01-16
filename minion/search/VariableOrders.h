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

  template<typename VarType = AnyVarRef>
  struct StaticVariableOrder
  {
    vector<VarType> var_order;
	vector<BOOL> val_order;
	vector<int> branches;
	unsigned pos;
	
	StaticVariableOrder(vector<VarType>& _varorder, vector<BOOL>& _valorder)
    : var_order(_varorder), val_order(_valorder)
	{
	  branches.reserve(1000);
	  pos = 0; 
	}
	
	// Returns true if all variables assigned
	BOOL find_next_unassigned()
	{
	  unsigned v_size = var_order.size();
	  while(pos < v_size && var_order[pos].isAssigned())
	    ++pos;
	  return pos == v_size;
	}
		
	BOOL finished_search()
	{ return branches.size() == 0; }
	
	void branch_left()
	{
	  D_ASSERT(!var_order[pos].isAssigned()) 
      int assign_val;
	  if(val_order[pos])
	    assign_val = var_order[pos].getMin();
	  else
		assign_val = var_order[pos].getMax();
	  var_order[pos].uncheckedAssign(assign_val);
	  
	  branches.push_back(pos);
	}
	
	void branch_right()
	{  
	   pos = branches.back();
	   branches.pop_back();
	   
	   if(val_order[pos])
	   {
		 D_ASSERT(var_order[pos].getMax() >= var_order[pos].getMin() + 1);
		 var_order[pos].setMin(var_order[pos].getMin() + 1);
	   }
	   else
	   {
		 D_ASSERT(var_order[pos].getMax() >= var_order[pos].getMin() + 1);
		 var_order[pos].setMax(var_order[pos].getMax() - 1);
	   }
	}
  };


  // Just a little helper function used in SDFVariableOrder
  inline bool is_boolean(const AnyVarRef& var)
  { return (var.getInitialMax() - var.getInitialMin()) == 1; } 

  template<typename VarType = AnyVarRef>
  struct SDFVariableOrder
  {
    vector<VarType> var_order;
	vector<BOOL> val_order;
	vector<int> branches;
	unsigned bool_end;
	unsigned pos;
	
	SDFVariableOrder(vector<AnyVarRef>& _varorder, vector<BOOL>& _valorder)
    : var_order(_varorder), val_order(_valorder)
	{
	  branches.reserve(1000);
	  pos = 0; 
	  bool_end = std::stable_partition(var_order.begin(), var_order.end(), is_boolean) - var_order.begin();
	}
	
	// Returns true if all variables assigned
	BOOL find_next_unassigned()
	{
	  // A special initial check just for Booleans.
	  while(pos < bool_end && var_order[pos].isAssigned())
	    ++pos;
	  if (pos < bool_end)
		return pos;
	  
	  // IMPORTANT:
	  // Remember, when there are two values remaining in the domain,
	  // max - min will equal ONE. When the domain is a single value,
	  // max - min will equal ZERO.
	  
	  unsigned v_size = var_order.size();
	  unsigned loop = bool_end;
	
	  // Set up values
	  int dom_size = INT_MAX;
	  int var_pos = -1;	
		
	  while(loop < v_size)
	  {
	    int new_size = var_order[loop].getMax() - var_order[loop].getMin();
		D_ASSERT(new_size >= 0);
		if( (new_size != 0) && (new_size < dom_size) )
		{
		  var_pos = loop;
		  
		  if(new_size == 1)
		  {
		    pos = var_pos;
			return false;
		  }
		  dom_size = new_size;
		}
	    ++loop;		
	  }
	  
	  if(var_pos == -1)
		return true;
	  else
	  {
	    pos = var_pos;
	    return false;
	  }
	}
		
	BOOL finished_search()
	{ return branches.size() == 0; }
	
    void branch_left()
	{
	  D_ASSERT(!var_order[pos].isAssigned()) 
      int assign_val;
	  if(val_order[pos])
	    assign_val = var_order[pos].getMin();
	  else
		assign_val = var_order[pos].getMax();
	  var_order[pos].uncheckedAssign(assign_val);
	  
	  branches.push_back(pos);
	}
	
	void branch_right()
	{  
	  pos = branches.back();
	  branches.pop_back();
	  
	  if(val_order[pos])
	  {
		D_ASSERT(var_order[pos].getMax() >= var_order[pos].getMin() + 1);
		var_order[pos].setMin(var_order[pos].getMin() + 1);
	  }
	  else
	  {
		D_ASSERT(var_order[pos].getMin() <= var_order[pos].getMin() - 1);
		var_order[pos].setMax(var_order[pos].getMax() - 1);
	  }
	}
  };
 

