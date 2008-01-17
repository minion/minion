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


/// (var1 = var2) = var3
template<typename EqualVarRef1, typename EqualVarRef2, typename BoolVarRef>
struct ReifiedEqualConstraint : public Constraint
{
  virtual string constraint_name()
  { return "ReifiedEqual"; }
  
  EqualVarRef1 var1;
  EqualVarRef2 var2;
  BoolVarRef var3;
  ReifiedEqualConstraint(EqualVarRef1 _var1, EqualVarRef2 _var2, BoolVarRef _var3) :
    var1(_var1), var2(_var2), var3(_var3)
  {}
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_ANDCON,"Setting up Constraint");
    triggerCollection t;
	t.push_back(make_trigger(var1, Trigger(this, 1), Assigned));
	t.push_back(make_trigger(var2, Trigger(this, 2), Assigned));
	t.push_back(make_trigger(var3, Trigger(this, 3), LowerBound));
	t.push_back(make_trigger(var3, Trigger(this, -3), UpperBound));
	return t;
  }
  
  virtual void full_propogate()
  {
    if(var3.isAssigned())
    {
      if(var3.getAssignedValue() == 1)
		propogate(3,0);
      else
		propogate(-3,0);
    }
    
    if(var1.isAssigned())
      propogate(1,0);
    
    if(var2.isAssigned())
      propogate(2,0);
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
	PROP_INFO_ADDONE(ReifyEqual);
    switch(i)
    {
      case 1:
		if(var2.isAssigned())
		{ var3.propogateAssign(var1.getAssignedValue() == var2.getAssignedValue()); }
		else
		{
		  if(var3.isAssigned())
		  {
			if(var3.getAssignedValue() == 1)
			{ var2.propogateAssign(var1.getAssignedValue()); }
		  }
		}
		break;
        
      case 2:
        if(var1.isAssigned())
		{ var3.propogateAssign(var1.getAssignedValue() == var2.getAssignedValue()); }
		else
		{
		  if(var3.isAssigned())
		  {
			if(var3.getAssignedValue() == 1)
			{ var1.propogateAssign(var2.getAssignedValue()); }
		  }
		}
		break;        
		
      case 3:
		if(var1.isAssigned())
		{ var2.propogateAssign(var1.getAssignedValue()); }
		else
		{
		  if(var2.isAssigned())
		  { var1.propogateAssign(var2.getAssignedValue()); }
		}
		break;
		
      case -3:
        if(var1.isAssigned() && var2.isAssigned())
		{ 
		  if(var1.getAssignedValue() == var2.getAssignedValue())
			Controller::fail();
		}
		break;
    }
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    D_ASSERT(v.size() == 3);
    D_ASSERT(v[2] == 0 || v[2] == 1);
    return (v[0] == v[1]) == v[2];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
	vars.reserve(3);
	vars.push_back(var1);
	vars.push_back(var2);
	vars.push_back(var3);
	return vars;
  }
};

template<typename EqualVarRef1, typename EqualVarRef2>
struct EqualConstraint : public Constraint
{
  virtual string constraint_name()
  { return "Equal"; }
  
  EqualVarRef1 var1;
  EqualVarRef2 var2;
  EqualConstraint(EqualVarRef1 _var1, EqualVarRef2 _var2) :
    var1(_var1), var2(_var2)
  {}
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_ANDCON,"Setting up Constraint");
    triggerCollection t;
	t.push_back(make_trigger(var1, Trigger(this, 1), UpperBound));
	t.push_back(make_trigger(var1, Trigger(this, 2), LowerBound));
	t.push_back(make_trigger(var2, Trigger(this, 3), UpperBound));
	t.push_back(make_trigger(var2, Trigger(this, 4), LowerBound));
	return t;
  }
  
  virtual void full_propogate()
  {
	propogate(1,0);
	propogate(2,0);
	propogate(3,0);
	propogate(4,0);
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
	PROP_INFO_ADDONE(Equal);
    switch(i)
	{
	  case 1:
		var2.setMax(var1.getMax());
		return;
	  case 2:
		var2.setMin(var1.getMin());
		return;
	  case 3:
		var1.setMax(var2.getMax());
		return;
	  case 4:
		var1.setMin(var2.getMin());
		return;
	}
  }
  
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    D_ASSERT(v.size() == 2);
    return (v[0] == v[1]);
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
	vars.reserve(2);
	vars.push_back(var1);
	vars.push_back(var2);
    return vars;
  }
  
};


template<typename EqualVarRef1, typename EqualVarRef2, typename BoolVarRef>
Constraint*
ReifiedEqualCon(EqualVarRef1 var1, EqualVarRef2 var2, BoolVarRef var3)
{ return new ReifiedEqualConstraint<EqualVarRef1, EqualVarRef2, BoolVarRef>(var1,var2,var3); }

template<typename EqualVarRef1, typename EqualVarRef2>
Constraint*
EqualCon(EqualVarRef1 var1, EqualVarRef2 var2)
{ return new EqualConstraint<EqualVarRef1, EqualVarRef2>(var1,var2); }


template<typename EqualVarRef1, typename EqualVarRef2, typename BoolVarRef>
Constraint*
ReifiedEqualMinusCon(EqualVarRef1 var1, EqualVarRef2 var2, BoolVarRef var3)
{ return new ReifiedEqualConstraint<EqualVarRef1, VarNeg<EqualVarRef2>, BoolVarRef>(var1,VarNegRef(var2),var3); }

template<typename EqualVarRef1, typename EqualVarRef2>
Constraint*
EqualMinusCon(EqualVarRef1 var1, EqualVarRef2 var2)
{ return new EqualConstraint<EqualVarRef1, VarNeg<EqualVarRef2> >(var1,VarNegRef(var2)); }


template<typename T1, typename T2>
Constraint*
BuildCT_EQ(const T1& t1, const T2& t2, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob&) 
{
  if(reify)
  { return ReifiedEqualCon(t1[0],t2[0], reifyVar); }
  else
  { return EqualCon(t1[0],t2[0]); }
}

template<typename T1, typename T2>
Constraint*
BuildCT_MINUSEQ(const T1& t1, const T2& t2, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob&) 
{
  if(reify)
  { return ReifiedEqualMinusCon(t1[0],t2[0], reifyVar); }
  else
  { return EqualMinusCon(t1[0],t2[0]); }
}
