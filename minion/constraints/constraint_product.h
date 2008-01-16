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

/// var1 * var2 = var3
template<typename VarRef1, typename VarRef2, typename VarRef3>
struct ProductConstraint : public Constraint
{
  virtual string constraint_name()
  { return "Product"; }
  
  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;
  ProductConstraint(VarRef1 _var1, VarRef2 _var2, VarRef3 _var3) :
	var1(_var1), var2(_var2), var3(_var3)
  {}
  
  virtual triggerCollection setup_internal()
  {
	D_INFO(2,DI_ANDCON,"Setting up Constraint");
	triggerCollection t;
	t.push_back(make_trigger(var1, Trigger(this, 1), LowerBound));
	t.push_back(make_trigger(var2, Trigger(this, 2), LowerBound));
	t.push_back(make_trigger(var3, Trigger(this, 3), LowerBound));
	t.push_back(make_trigger(var1, Trigger(this, 1), UpperBound));
	t.push_back(make_trigger(var2, Trigger(this, 2), UpperBound));
	t.push_back(make_trigger(var3, Trigger(this, 3), UpperBound));
	return t;
  }
  
  int mult_max(int min1, int max1, int min2, int max2)
  { return mymax(mymax(min1*min2, min1*max2),mymax(max1*min2, max1*max2)); }
  
  int mult_min(int min1, int max1, int min2, int max2)
  { return mymin(mymin(min1*min2, min1*max2),mymin(max1*min2, max1*max2)); }
  
  int round_up_div(int x, int y)
  {
	if(y == 0)
	  return 0;
	div_t division = div(x,y);
	int return_val = division.quot;
	if(division.rem != 0)
	  return_val++;
	return return_val;
  }
  
  int round_down_div(int x, int y)
  { 
	if(y == 0)
	  return INT_MAX;
	return x / y; 
  }
  
  PROPAGATE_FUNCTION(int, DomainDelta)
  {
	int var1_min = var1.getMin();
	int var1_max = var1.getMax();
	int var2_min = var2.getMin();
	int var2_max = var2.getMax();
	int var3_min = var3.getMin();
	int var3_max = var3.getMax();
	
	if((var1_min >= 0) && (var2_min >= 0))
	{
	  // We don't have to deal with negative numbers. yay!
	  
	  var3_min = max(var3_min, var1_min * var2_min);
	  var3_max = min(var3_max, var1_max * var2_max);
	  
	  var1_min = max(var1_min, round_up_div(var3_min, var2_max));
	  var1_max = min(var1_max, round_down_div(var3_max, var2_min));
	  
	  var2_min = max(var2_min, round_up_div(var3_min, var1_max));
	  var2_max = min(var2_max, round_down_div(var3_max, var1_min));
	  
	  var1.setMin(var1_min);
	  var1.setMax(var1_max);
	  var2.setMin(var2_min);
	  var2.setMax(var2_max);
	  var3.setMin(var3_min);
	  var3.setMax(var3_max);
	}
    else
	{
	  var3.setMax(mult_max(var1_min, var1_max, var2_min, var2_max));
	  var3.setMin(mult_min(var1_min, var1_max, var2_min, var2_max));
	  if(var1.isAssigned())
  	  {
	    int val1 = var1.getAssignedValue();
	    if(val1 > 0)
	    { 
		  var3.setMin(var2.getMin() * val1);
		  var3.setMax(var2.getMax() * val1);
		}
		else
		{
		  var3.setMin(var2.getMax() * val1);
		  var3.setMax(var2.getMin() * val1);
		}
	  }
	  
	  if(var2.isAssigned())
	  {
		int val2 = var2.getAssignedValue();
		if(val2 > 0)
		{ 
		  var3.setMin(var1.getMin() * val2);
		  var3.setMax(var1.getMax() * val2);
		}
		else
		{
		  var3.setMin(var1.getMax() * val2);
		  var3.setMax(var1.getMin() * val2);
		}
	  }
	}
  }
  
  virtual void full_propogate()
  { propogate(0,0); }
  
  virtual BOOL check_assignment(vector<int> v)
  {
	D_ASSERT(v.size() == 3);
	return (v[0] * v[1]) == v[2];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> v;
	v.push_back(var1);
	v.push_back(var2);
	v.push_back(var3);
	return v;
  }
};

#ifndef NO_SPECIALISATIONS

inline Constraint*
ProductCon(const vector<BoolVarRef>& vars, const vector<BoolVarRef>& var2)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return AndCon(vars[0], vars[1], var2[0]);
}

#endif

template<typename VarRef1, typename VarRef2>
Constraint*
ProductCon(const vector<VarRef1>& vars, const vector<VarRef2>& var2)
{ 
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return new ProductConstraint<VarRef1,VarRef1,VarRef2>(vars[0],vars[1],var2[0]); 
}

BUILD_CONSTRAINT2(CT_PRODUCT2, ProductCon);
