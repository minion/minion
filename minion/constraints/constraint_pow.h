/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: constraint_product.h 472 2006-11-17 17:04:36Z azumanga $
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


#include <math.h>

/// var1 ^ var2 = var3
template<typename VarRef1, typename VarRef2, typename VarRef3>
struct PowConstraint : public Constraint
{
  virtual string constraint_name()
  { return "Product"; }
  
  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;

  PowConstraint(VarRef1 _var1, VarRef2 _var2, VarRef3 _var3) :
	var1(_var1), var2(_var2), var3(_var3)
  {
  
	  if(var1.getInitialMin() < 0 || var2.getInitialMin() < 0 ||
		 var3.getInitialMin() < 0)
	  { 
		cerr << "The 'pow' constraint only supports positive numbers at present.";
		exit(1);
	  }
  }
  
  virtual triggerCollection setup_internal()
  {
	D_INFO(2,DI_ANDCON,"Setting up Constraint");
	triggerCollection t;
	t.push_back(make_trigger(var1, Trigger(this, -1), LowerBound));
	t.push_back(make_trigger(var2, Trigger(this, -2), LowerBound));
	t.push_back(make_trigger(var3, Trigger(this, -3), LowerBound));
	t.push_back(make_trigger(var1, Trigger(this, 1), UpperBound));
	t.push_back(make_trigger(var2, Trigger(this, 2), UpperBound));
	t.push_back(make_trigger(var3, Trigger(this, 3), UpperBound));
	return t;
  }
    
  
  double my_pow(double x, double y)
  { return pow(x,y);}
  
  double my_y(double x, double z)
  { return log(z) / log(x); }
  
  double my_x(double y, double z)
  { return exp(log(z) / y); }
  
  PROPAGATE_FUNCTION(int flag, DomainDelta)
  {
	
	switch(flag)
	{
	  case -1:
	  {
		var3.setMin(lrint(my_pow(var1.getMin(),var2.getMin())));
		int var1_min = var1.getMin();
		if(var1_min > 1)
		  var2.setMax(lrint(my_y(var1_min, var3.getMax())));
		break;
	  }
	  case -2:
	    var3.setMin(lrint(my_pow(var1.getMin(), var2.getMin())));
		var1.setMax(lrint(my_x(var2.getMin(), var3.getMax())));
		break;
		
	  case -3:
	  {
		var1.setMin(lrint(my_x(var2.getMax(), var3.getMin())));
		int var1_max = var1.getMax();
		if(var1_max > 1)
		  var2.setMin(lrint(my_y(var1_max, var3.getMin())));
		break;
	  }
	  case 1:
	  {
		var3.setMax(lrint(my_pow(var1.getMax(),var2.getMax())));
		int var1_max = var1.getMax();
		if(var1_max > 1)
		  var2.setMin(lrint(my_y(var1_max, var3.getMin())));
		break;
	  }
	  case 2:
	    var3.setMax(lrint(my_pow(var1.getMax(), var2.getMax())));
		var1.setMin(lrint(my_x(var2.getMax(), var3.getMin())));
		break;
		
	  case 3:
	  {
		var1.setMax(lrint(my_x(var2.getMin(), var3.getMax())));
		int var1_min = var1.getMin();
		if(var1_min > 1)
		  var2.setMax(lrint(my_y(var1_min, var3.getMax())));
		break;
	  }
	}
  }
  
  virtual void full_propogate()
  { 
	propogate(1,0); 
    propogate(2,0);
    propogate(3,0);
    propogate(-1,0);
    propogate(-2,0);
    propogate(-3,0);
  }
  
  virtual BOOL check_assignment(vector<int> v)
  {
	D_ASSERT(v.size() == 3);
	return my_pow(v[0],v[1]) == v[2];
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

template<typename V1, typename V2>
inline Constraint*
PowCon(const V1& vars, const V2& var2)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return new PowConstraint<typename V1::value_type, typename V1::value_type,
						   typename V2::value_type>(vars[0], vars[1], var2[0]);
}


BUILD_CONSTRAINT2(CT_POW, PowCon);
