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

// This constraint is half-way to being changed from using
// LRINT to roundup and rounddown. Still don't quite have my head around
// this +0.5 business. Or at least I'm not convinced that it's OK.
// at least now it passes test_nightingale_pow.minion.

#define LRINT(x) static_cast<DomainInt>(x + 0.5)

/// var1 ^ var2 = var3
template<typename VarRef1, typename VarRef2, typename VarRef3>
struct PowConstraint : public Constraint
{
  virtual string constraint_name()
  { return "Product"; }
  
  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;

  PowConstraint(StateObj* _stateObj, VarRef1 _var1, VarRef2 _var2, VarRef3 _var3) :
	Constraint(_stateObj), var1(_var1), var2(_var2), var3(_var3)
  {
  
	  if(var1.getInitialMin() < 0 || var2.getInitialMin() < 0 ||
		 var3.getInitialMin() < 0)
	  { 
		cerr << "The 'pow' constraint only supports non-negative numbers at present.";
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
  
  inline DomainInt roundup(double x)
  {
    // remember no numbers are non-negative in here, so
    // how are we going to hit the lower limit for ints?
    if(x<std::numeric_limits<DomainInt>::min())
    {
      return std::numeric_limits<DomainInt>::min();
    }
    else
    {
      return static_cast<DomainInt>(x);  // Actually this should round up!
    }
}
    
  inline DomainInt rounddown(double x)
  {
    if(x>std::numeric_limits<DomainInt>::max())
    {
      return std::numeric_limits<DomainInt>::max();
    }
    else
    {
      return static_cast<DomainInt>(x);  
    }
  }
  
  
  double my_pow(DomainInt x, DomainInt y)
  { return pow((double)checked_cast<int>(x), checked_cast<int>(y));}
  
  double my_y(DomainInt x, DomainInt z)
  { return log((double)checked_cast<int>(z)) / log((double)checked_cast<int>(x)); }
  
  double my_x(DomainInt y, DomainInt z)
  { return exp(log((double)checked_cast<int>(z)) / checked_cast<int>(y)); }
  
  PROPAGATE_FUNCTION(int flag, DomainDelta)
  {
	PROP_INFO_ADDONE(Pow);
	switch(flag)
	{
	  case -1:
	  {
        // var3 >= min(var1) ^ min(var2)
		var3.setMin(LRINT(my_pow(var1.getMin(),var2.getMin())));
		DomainInt var1_min = var1.getMin();
		if(var1_min > 1)
          // var2 <= log base max(var3) of min(var1)
		  var2.setMax(LRINT(my_y(var1_min, var3.getMax())));
		break;
	  }
	  case -2:
        // var3>= min(var1) ^ min(var2) 
	    var3.setMin(LRINT(my_pow(var1.getMin(), var2.getMin())));
		var1.setMax(LRINT(my_x(var2.getMin(), var3.getMax())));
		break;
		
	  case -3:
	  {
		var1.setMin(LRINT(my_x(var2.getMax(), var3.getMin())));
		DomainInt var1_max = var1.getMax();
		if(var1_max > 1)
		  var2.setMin(LRINT(my_y(var1_max, var3.getMin())));
		break;
	  }
	  case 1:
	  {
		var3.setMax(rounddown(my_pow(var1.getMax(),var2.getMax())));  // wraparound was occurring here, so use rounddown
		DomainInt var1_max = var1.getMax();
		if(var1_max > 1)
		  var2.setMin(LRINT(my_y(var1_max, var3.getMin())));
		break;
	  }
	  case 2:
	    var3.setMax(rounddown(my_pow(var1.getMax(), var2.getMax())));  // wraparound here.
		var1.setMin(LRINT(my_x(var2.getMax(), var3.getMin())));
		break;
		
	  case 3:
	  {
		var1.setMax(LRINT(my_x(var2.getMin(), var3.getMax())));
		DomainInt var1_min = var1.getMin();
		if(var1_min > 1)
		  var2.setMax(LRINT(my_y(var1_min, var3.getMax())));
		break;
	  }
	}
  }
  
  virtual void full_propagate()
  { 
    propagate(1,0); 
    propagate(2,0);
    propagate(3,0);
    propagate(-1,0);
    propagate(-2,0);
    propagate(-3,0);
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
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
PowCon(StateObj* stateObj, const V1& vars, const V2& var2)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return new PowConstraint<typename V1::value_type, typename V1::value_type,
						   typename V2::value_type>(stateObj, vars[0], vars[1], var2[0]);
}


BUILD_CONSTRAINT2(CT_POW, PowCon);
