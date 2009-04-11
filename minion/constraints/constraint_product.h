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

/** @help constraints;product Description
The constraint

   product(x,y,z)

ensures that z=xy in any solution.
*/

/** @help constraints;product Notes
This constraint can be used for (and, in fact, has a specialised
implementation for) achieving boolean AND, i.e. x & y=z can be modelled
as

   product(x,y,z)

The general constraint achieves bounds generalised arc consistency for
positive numbers.
*/

#ifndef CONSTRAINT_PRODUCT_H
#define CONSTRAINT_PRODUCT_H

#include "constraint_and.h"

/// var1 * var2 = var3
template<typename VarRef1, typename VarRef2, typename VarRef3>
struct ProductConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Product"; }
  
  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;
  ProductConstraint(StateObj* _stateObj, VarRef1 _var1, VarRef2 _var2, VarRef3 _var3) :
    AbstractConstraint(_stateObj), var1(_var1), var2(_var2), var3(_var3)
  { }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    t.push_back(make_trigger(var1, Trigger(this, 1), LowerBound));
    t.push_back(make_trigger(var2, Trigger(this, 2), LowerBound));
    t.push_back(make_trigger(var3, Trigger(this, 3), LowerBound));
    t.push_back(make_trigger(var1, Trigger(this, 1), UpperBound));
    t.push_back(make_trigger(var2, Trigger(this, 2), UpperBound));
    t.push_back(make_trigger(var3, Trigger(this, 3), UpperBound));
    return t;
  }
  
  DomainInt mult_max(DomainInt min1, DomainInt max1, DomainInt min2, DomainInt max2)
  { return mymax(mymax(min1*min2, min1*max2),mymax(max1*min2, max1*max2)); }
  
  DomainInt mult_min(DomainInt min1, DomainInt max1, DomainInt min2, DomainInt max2)
  { return mymin(mymin(min1*min2, min1*max2),mymin(max1*min2, max1*max2)); }
  
  DomainInt round_up_div(DomainInt x, DomainInt y)
  {
    if(y == 0)
      return 0;
    ldiv_t division = div(checked_cast<long>(x), checked_cast<long>(y));
    long return_val = division.quot;
    if(division.rem != 0)
      return_val++;
    return return_val;
  }
  
  DomainInt round_down_div(DomainInt x, DomainInt y)
  { 
    if(y == 0)
      return DomainInt_Max;
    return x / y; 
  }
  
  virtual void propagate(int, DomainDelta)
  {
    PROP_INFO_ADDONE(Product);
    DomainInt var1_min = var1.getMin();
    DomainInt var1_max = var1.getMax();
    DomainInt var2_min = var2.getMin();
    DomainInt var2_max = var2.getMax();
    DomainInt var3_min = var3.getMin();
    DomainInt var3_max = var3.getMax();
    
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
        DomainInt val1 = var1.getAssignedValue();
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
        DomainInt val2 = var2.getAssignedValue();
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
  
  virtual void full_propagate()
  { propagate(0,0); }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 3);
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
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
    {  
      for(DomainInt v1 = var1.getMin(); v1 <= var1.getMax(); ++v1)
      {
        if(var1.inDomain(v1))
        {
          for(DomainInt v2 = var2.getMin(); v2 <= var2.getMax(); ++v2)
          {
            if(var2.inDomain(v2) && var3.inDomain(v1 * v2))
            {
              assignment.push_back(make_pair(0, v1));
              assignment.push_back(make_pair(1, v2));
              assignment.push_back(make_pair(2, v1 * v2));
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
      return new CheckAssignConstraint<vector<AnyVarRef>, ProductConstraint>(stateObj, t, *this);
  }
};



inline AbstractConstraint*
BuildCT_PRODUCT2(StateObj* stateObj, const vector<BoolVarRef>& vars, const vector<BoolVarRef>& var2, ConstraintBlob&)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return AndCon(stateObj, vars[0], vars[1], var2[0]);
}

template<typename VarRef1, typename VarRef2>
AbstractConstraint*
BuildCT_PRODUCT2(StateObj* stateObj, const vector<VarRef1>& vars, const vector<VarRef2>& var2, ConstraintBlob&)
{ 
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return new ProductConstraint<VarRef1,VarRef1,VarRef2>(stateObj, vars[0],vars[1],var2[0]); 
}

#endif
