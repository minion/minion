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

#ifndef CONSTRAINT_DYNAMIC_BINARY_SAT_H
#define CONSTRAINT_DYNAMIC_BINARY_SAT_H

//! Specialised SAT implementation for just 2 variables
template<typename VarArray>
struct BoolBinarySATConstraintDynamic : public AbstractConstraint
{
  virtual string constraint_name()
  { return "BoolBinarySATDynamic"; }
  
  typedef typename VarArray::value_type VarRef;
  
  VarRef var1;
  VarRef var2;
  
  BoolBinarySATConstraintDynamic(StateObj* _stateObj,const VarArray& _var_array) :
    AbstractConstraint(_stateObj), var1(_var_array[0]), var2(_var_array[1])
  { D_ASSERT(_var_array.size() == 2); }
  
  int dynamic_trigger_count()
  {
    return 2;
  }
    
  virtual void full_propagate()
  {
    DynamicTrigger* dt = dynamic_trigger_start();

    if(var1.isAssignedValue(false))
    {
      var2.propagateAssign(true);
      return;
    }
    
    if(var2.isAssignedValue(false))
    {
      var1.propagateAssign(true);
      return;
    }
    
    dt->trigger_info() = 0;
    var1.addDynamicTrigger(dt, UpperBound);
    
    ++dt;
    
    dt->trigger_info() = 1;
    var2.addDynamicTrigger(dt, UpperBound);
    
    return;
  }
    
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
    PROP_INFO_ADDONE(Dyn2SAT);
    int propval = dt->trigger_info();
    
    if(propval)
      var1.propagateAssign(true);
    else
      var2.propagateAssign(true);
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    return (v[0] != 0) || (v[1] != 0);
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(2);
    vars.push_back(var1);
    vars.push_back(var2);
    return vars;  
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    if(var1.getMax() > 0)
    {
      assignment.push_back(make_pair(0, var1.getMax()));
      return true;
    }
    
    if(var2.getMax() > 0)
    {
      assignment.push_back(make_pair(1, var2.getMax()));
      return true;
    }
    return false;
  }
};

template<typename VarArray>
AbstractConstraint*
BoolBinarySATConDynamic(StateObj* stateObj, const VarArray& _var_array)
{ return new BoolBinarySATConstraintDynamic<VarArray>(stateObj, _var_array); }

#endif
