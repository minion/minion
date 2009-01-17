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

/** @help constraints;div Description
The constraint
 
   div(x,y,z)

ensures that floor(x/y)=z.
*/

/** @help constraints;div Notes
This constraint is only available for positive domains x, y and z.
*/

/** @help constraints;div Reifiability
This constraint is reifyimply'able but not reifiable.
*/

/** @help constraints;div References
help constraints modulo
*/

#ifndef CONSTRAINT_DIV_H
#define CONSTRAINT_DIV_H

#include <math.h>

#ifndef LRINT
#define LRINT(x) static_cast<DomainInt>(x + 0.5)
#endif

/// var1 / var2 = var3
template<typename VarRef1, typename VarRef2, typename VarRef3>
struct DivConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Div"; }
  
  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;

  DivConstraint(StateObj* _stateObj, VarRef1 _var1, VarRef2 _var2, VarRef3 _var3) : AbstractConstraint(_stateObj),
	var1(_var1), var2(_var2), var3(_var3)
  {
  
	  if(var1.getInitialMin() < 0 || var2.getInitialMin() < 0 ||
		 var3.getInitialMin() < 0)
	  { 
      FAIL_EXIT("The 'div' constraint only supports positive numbers at present.");
	  }
  }
  
  virtual triggerCollection setup_internal()
  {
	triggerCollection t;
	t.push_back(make_trigger(var1, Trigger(this, -1), LowerBound));
	t.push_back(make_trigger(var2, Trigger(this, -2), LowerBound));
	t.push_back(make_trigger(var3, Trigger(this, -3), LowerBound));
	t.push_back(make_trigger(var1, Trigger(this, 1), UpperBound));
	t.push_back(make_trigger(var2, Trigger(this, 2), UpperBound));
	t.push_back(make_trigger(var3, Trigger(this, 3), UpperBound));
	return t;
  }
    
  PROPAGATE_FUNCTION(int flag, DomainDelta)
  {
    PROP_INFO_ADDONE(Pow);
    if(var1.isAssigned() && var2.isAssigned())
	  {
	    if(var2.getAssignedValue() == 0)
	      getState(stateObj).setFailed(true);
      var3.propagateAssign(var1.getAssignedValue() / var2.getAssignedValue() );
    }
  }
  
  virtual void full_propagate()
  { 
    if(!var2.isBound())
      var2.removeFromDomain(0);
      
    propagate(1,0); 
    propagate(2,0);
    propagate(3,0);
    propagate(-1,0);
    propagate(-2,0);
    propagate(-3,0);
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
	D_ASSERT(v_size == 3);
	return v[0] / v[1] == v[2];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> v;
	v.push_back(var1);
	v.push_back(var2);
	v.push_back(var3);
	return v;
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {  
   for(DomainInt v1 = var1.getMin(); v1 <= var1.getMax(); ++v1)
   {
     if(var1.inDomain(v1))
     {
       for(DomainInt v2 = var2.getMin(); v2 <= var2.getMax(); ++v2)
       {
         if(var2.inDomain(v2) && var3.inDomain(v1 / v2))
         {
           assignment.push_back(make_pair(0, v1));
           assignment.push_back(make_pair(1, v2));
           assignment.push_back(make_pair(2, v1 / v2));
           return true;
         }
       }
     }
   }
   return false;
  }
 
     // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverse_constraint()
  {
      vector<AnyVarRef> t;
      t.push_back(var1);
      t.push_back(var2);
      t.push_back(var3);
      return new CheckAssignConstraint<vector<AnyVarRef>, DivConstraint>(stateObj, t, *this);
  }
};

template<typename V1, typename V2>
inline AbstractConstraint*
BuildCT_DIV(StateObj* stateObj, const V1& vars, const V2& var2, ConstraintBlob&)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return new DivConstraint<typename V1::value_type, typename V1::value_type,
						   typename V2::value_type>(stateObj, vars[0], vars[1], var2[0]);
}

#endif
