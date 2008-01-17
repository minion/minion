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


// x <= y + offset
template<typename VarRef1, typename VarRef2, typename Offset>
struct LeqConstraint : public Constraint
{
  virtual string constraint_name()
  { return "Leq"; }
  
  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  const Offset offset;
  VarRef1 x;
  VarRef2 y;
  
  LeqConstraint(VarRef1 _x, VarRef2 _y, Offset _o) :
    offset(_o), x(_x), y(_y)
  { }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
	t.push_back(make_trigger(x, Trigger(this, 0), LowerBound));
	t.push_back(make_trigger(y, Trigger(this, 1), UpperBound));
	return t;
  }

  // Needs to be at end of file
  virtual Constraint* reverse_constraint();
  
  PROPAGATE_FUNCTION(int prop_val,DomainDelta)
  {
	PROP_INFO_ADDONE(BinaryLeq);
    if(prop_val)
    {// y changed
      x.setMax(y.getMax() + offset);
    }
    else
    {// x changed
      y.setMin(x.getMin() - offset);
    }
  }
  
  virtual BOOL check_unsat(int,DomainDelta)
  { return (x.getMin() > y.getMax() + offset); }
  
  virtual BOOL full_check_unsat()
  { return (x.getMin() > y.getMax() + offset); }
  
  virtual void full_propagate()
  {
    propagate(0,0);
    propagate(1,0);
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
	D_ASSERT(v.size() == 2);
	return v[0] <= (v[1] + offset);
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
	vector<AnyVarRef> array;
	array.reserve(2);
	array.push_back(x);
	array.push_back(y);
	return array;
  }
};

template<typename VarRef1, typename VarRef2, typename Offset>
Constraint*
LeqCon(VarRef1 v1, VarRef2 v2, Offset o)
{ return new LeqConstraint<VarRef1,VarRef2,Offset>(v1,v2,o); }

template<typename VarRef1, typename VarRef2>
Constraint*
LeqCon(VarRef1 v1, VarRef2 v2)
{ return new LeqConstraint<VarRef1,VarRef2,compiletime_val<0> >(v1,v2,compiletime_val<0>()); }

template<typename VarRef>
Constraint*
ImpliesCon(VarRef v1, VarRef v2)
{ return new LeqConstraint<VarRef,VarRef,compiletime_val<0> >(v1,v2,compiletime_val<0>()); }

template<typename T1, typename T2>
Constraint*
BuildCT_INEQ(const T1& t1, const T2& t2, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b) 
{
  D_ASSERT(b.vars[2].size() == 1 && b.vars[2][0].type == VAR_CONSTANT);
  
  if(reify)
  { return reifyCon(LeqCon(t1[0], t2[0], runtime_val(b.vars[2][0].pos)), reifyVar); }
  else
  { return LeqCon(t1[0], t2[0], runtime_val(b.vars[2][0].pos)); }
}

// This is mainly inline to avoid multiple definitions.
template<typename VarRef1, typename VarRef2, typename Offset>
inline Constraint* LeqConstraint<VarRef1, VarRef2, Offset>::reverse_constraint()
{ return LeqCon(y,x, offset.negminusone()); }
