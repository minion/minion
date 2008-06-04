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

/** @help constraints;difference Description
The constraint

   difference(x,y,z)

ensures that z=|x-y| in any solution.
*/

/** @help constraints;product Notes
This constraint can be expressed in a much longer form, this form both avoids requiring an extra
variable, and also gets better propagation. It gets bounds consistency.
*/

/** @help constraints;product Reifiability
This constraint is not reifiable.
*/


/// |var1 - var2| = var3
template<typename VarRef1, typename VarRef2, typename VarRef3>
struct DifferenceConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Difference"; }
  
  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;
  DifferenceConstraint(StateObj* _stateObj, VarRef1 _var1, VarRef2 _var2, VarRef3 _var3) :
	AbstractConstraint(_stateObj), var1(_var1), var2(_var2), var3(_var3)
  { D_FATAL_ERROR("The Difference constraint isn't finished yet!"); }
  
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
    
  PROPAGATE_FUNCTION(int, DomainDelta)
  {
	  PROP_INFO_ADDONE(Difference);
	  DomainInt var1_min = var1.getMin();
	  DomainInt var1_max = var1.getMax();
	  DomainInt var2_min = var2.getMin();
	  DomainInt var2_max = var2.getMax();

    if(var1_max < var2_min)
    {
      var3.setMin(var2_min - var1_max);
      var2.setMin(var1.getMin() + var3.getMin());
      var1.setMax(var2.getMax() - var3.getMin());
    }

    if(var2_max < var1_min)
    {
      var3.setMin(var1_min - var2_max);
      var1.setMin(var2.getMin() + var3.getMin());
      var2.setMax(var1.getMax() - var3.getMin());
    }
      
    
    
      
	

  }
  
  virtual void full_propagate()
  { var3.setMin(0); }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
	  D_ASSERT(v_size == 3);
    int abs_val = v[0] - v[1];
    if(abs_val < 0) abs_val = - abs_val;
	  return abs_val == v[2];
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


template<typename VarRef1, typename VarRef2>
AbstractConstraint*
DifferenceCon(StateObj* stateObj,const light_vector<VarRef1>& vars, const light_vector<VarRef2>& var2)
{ 
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return new DifferenceConstraint<VarRef1,VarRef1,VarRef2>(stateObj, vars[0], vars[1], var2[0]); 
}

BUILD_CONSTRAINT2(CT_DIFFERENCE, DifferenceCon);
