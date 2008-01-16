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
  Offset offset;
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
  
  //  virtual Constraint* reverse_constraint()
  
  PROPAGATE_FUNCTION(int prop_val,DomainDelta)
  {
    if(prop_val)
    {// y changed
      x.setMax(y.getMax() + offset.val());
    }
    else
    {// x changed
      y.setMin(x.getMin() - offset.val());
    }
  }
  
  virtual bool check_unsat(int,DomainDelta)
  { return (x.getMin() > y.getMax() + offset.val()); }
  
  virtual void full_propogate()
  {
    propogate(0,0);
    propogate(1,0);
  }
  
  virtual bool check_assignment(vector<int> v)
  {
	D_ASSERT(v.size() == 2);
	return ((int)(v[0]) <= ((int)v[1] + offset.val()));
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


