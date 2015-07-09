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
  { return "watchless"; }

  Var1 var1;
  Var2 var2;

  CONSTRAINT_ARG_LIST2(var1, var2);

  WatchLessConstraint(const Var1& _var1, const Var2& _var2) :
    var1(_var1), var2(_var2)
  { }

  virtual SysInt dynamic_trigger_count()
  { return 2; }

  virtual void full_propagate()
  {
    moveTriggerInt(var1, 0, LowerBound);
    moveTriggerInt(var2, 1, UpperBound);

    var2.setMin(var1.getMin() + 1);
    var1.setMax(var2.getMax() - 1);
  }


  virtual void propagateDynInt(SysInt  dt)
  {
      PROP_INFO_ADDONE(WatchNEQ);

      D_ASSERT(dt ==0 || dt == 1);

      if(dt == 0)
      { var2.setMin(var1.getMin() + 1); }
      else
      { var1.setMax(var2.getMax() - 1); }
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
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

  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
    if(var1.getMin() < var2.getMax())
    {
      assignment.push_back(make_pair(0,var1.getMin()));
      assignment.push_back(make_pair(1,var2.getMax()));
      return true;
    }
    return false;
  }

  template<bool b, typename T>
  typename std::enable_if<b, AbstractConstraint*>::type
   rev_implement(const ShiftVar<T,compiletime_val<SysInt, 1> >& var2)
  {
    return new WatchLessConstraint<T, Var1, false>(var2.data, var1);
  }

 template<bool b, typename T>
  typename std::enable_if<b, AbstractConstraint*>::type
   rev_implement(const T& var2)
  {
    return new WatchLessConstraint<AnyVarRef, AnyVarRef, true>(var2, ShiftVar<Var1,compiletime_val<SysInt, 1> >(var1, compiletime_val<SysInt, 1>()));
  }

  template<bool b,typename T>
  typename std::enable_if<!b, AbstractConstraint*>::type rev_implement(const T& var2)
  { return new WatchLessConstraint<Var2,ShiftVar<Var1,compiletime_val<SysInt, 1> >,true>(var2, ShiftVar<Var1,compiletime_val<SysInt, 1> >(var1, compiletime_val<SysInt, 1>())); }

  virtual AbstractConstraint* reverse_constraint() { return rev_implement<Negated>(var2); }
};

template<typename VarArray1, typename VarArray2>
AbstractConstraint*
BuildCT_WATCHED_LESS(const VarArray1& _var_array_1, const VarArray2& _var_array_2, ConstraintBlob&)
{
  return new WatchLessConstraint<typename VarArray1::value_type, typename VarArray2::value_type>
    (_var_array_1[0], _var_array_2[0]);
}

/* JSON
  { "type": "constraint",
    "name": "watchless",
    "internal_name": "CT_WATCHED_LESS",
    "args": [ "read_var", "read_var" ]
  }
*/
#endif
