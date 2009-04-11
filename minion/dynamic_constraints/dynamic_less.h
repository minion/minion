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

/** @help constraints;watchless Description
The constraint watchless(x,y) ensures that x is less than y.
*/

/** @help constraints;watchless References
  See also

  help constraints ineq
*/

#ifndef CONSTRAINT_DYNAMIC_LESS_H
#define CONSTRAINT_DYNAMIC_LESS_H

// var1 < var2
template<typename Var1, typename Var2, bool Negated=false>
struct WatchLessConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "WatchedLess"; }
  
  Var1 var1;
  Var2 var2;

  WatchLessConstraint(StateObj* _stateObj, const Var1& _var1, const Var2& _var2) :
    AbstractConstraint(_stateObj), var1(_var1), var2(_var2)
  { }
  
  int dynamic_trigger_count()
  { return 2; }
  
  virtual void full_propagate()
  {  
    DynamicTrigger* dt = dynamic_trigger_start();
        
    var1.addDynamicTrigger(dt    , LowerBound);
    var2.addDynamicTrigger(dt + 1, UpperBound);
    
    var2.setMin(var1.getMin() + 1);
    var1.setMax(var2.getMax() - 1);
  }
  
    
  virtual void propagate(DynamicTrigger* dt)
  {
      PROP_INFO_ADDONE(WatchNEQ);
      DynamicTrigger* dt_start = dynamic_trigger_start();
      
      D_ASSERT(dt == dt_start || dt == dt_start + 1);
    
      if(dt == dt_start)
      { var2.setMin(var1.getMin() + 1); }
      else
      { var1.setMax(var2.getMax() - 1); }
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 2);
    return v[0] < v[1];
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
    if(var1.getMin() < var2.getMax())
    {
      assignment.push_back(make_pair(0,var1.getMin()));
      assignment.push_back(make_pair(1,var2.getMax()));
      return true;
    }
    return false;
  }

  template<bool b>
  typename disable_if_c<b, AbstractConstraint*>::type rev_implement()
  { return new WatchLessConstraint<Var2,ShiftVar<Var1,DomainInt>,true>(stateObj, var2, ShiftVar<Var1,DomainInt>(var1, 1)); }

  template<bool b>
  typename enable_if_c<b, AbstractConstraint*>::type rev_implement()
  { FAIL_EXIT(); } //cannot reverse a reversed constraint

  virtual AbstractConstraint* reverse_constraint() { return rev_implement<Negated>(); }
};

template<typename VarArray1, typename VarArray2>
AbstractConstraint*
BuildCT_WATCHED_LESS(StateObj* stateObj, const VarArray1& _var_array_1, const VarArray2& _var_array_2, ConstraintBlob&)
{ 
  return new WatchLessConstraint<typename VarArray1::value_type, typename VarArray2::value_type>
    (stateObj, _var_array_1[0], _var_array_2[0]); 
}

#endif
