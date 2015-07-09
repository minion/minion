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

/** @help constraints;w-notliteral Description
  The constraint w-notliteral(x, a) ensures that x =/= a.
*/

/** @help constraints;w-notliteral References
  See also

  help constraints w-literal
*/

#ifndef CONSTRAINT_DYNAMIC_UNARY_NOT_INLITERAL_H
#define CONSTRAINT_DYNAMIC_UNARY_NOT_INLITERAL_H

// Checks if a variable is equal to a value.
template<typename Var>
  struct WatchNotLiteralConstraint : public AbstractConstraint
{
  virtual string constraint_name() { return "w-notliteral"; }

  Var var;

  DomainInt val;

  CONSTRAINT_ARG_LIST2(var, val);

  template<typename T>
  WatchNotLiteralConstraint(const Var& _var, const T& _val) :
    var(_var), val(_val) {}

  virtual SysInt dynamic_trigger_count()
  { return 1; }

  virtual void full_propagate()
  { 
    if(var.isBound())
    {
        if(var.getMin() == val)
            var.setMin(val + 1);
        else if(var.getMax() == val)
            var.setMax(val - 1);
        else
            moveTriggerInt(var, 0, DomainChanged);
    }
    else
      var.removeFromDomain(val); 
  }


  virtual void propagateDynInt(SysInt  dt)
  {
    PROP_INFO_ADDONE(WatchInRange);
    if(var.isBound())
    {
        if(var.getMin() == val)
            var.setMin(val + 1);
        else if(var.getMax() == val)
            var.setMax(val - 1);
    }
    else
      var.removeFromDomain(val); 
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    D_ASSERT(v_size == 1);
    return (v[0] != val);
  }

  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  { 
    D_ASSERT(var.inDomain(var.getMin()) && var.inDomain(var.getMax()));
    DomainInt tmp;
    if((tmp = var.getMin()) != val)
    {
        assignment.push_back(make_pair(0, tmp));
        return true;
    }
    else if((tmp = var.getMax()) != val)
    {
        assignment.push_back(make_pair(0, tmp));
        return true;
    }
    return false;
  }

   AbstractConstraint* reverse_constraint();
};

struct WatchNotLiteralBoolConstraint : public AbstractConstraint
{
   virtual string constraint_name() { return "w-notliteral"; }

  CONSTRAINT_ARG_LIST2(var, val);

  BoolVarRef var;

  DomainInt val;

  template<typename T>
  WatchNotLiteralBoolConstraint(const BoolVarRef& _var, const T& _val) :
    var(_var), val(_val) 
  {
    //cout << "using boolean specialisation" << endl;
  }  

  virtual SysInt dynamic_trigger_count()
  { return 0; }

  virtual void full_propagate()
  { 
    var.removeFromDomain(val); 
  }

  virtual void propagateDynInt(SysInt  dt)
  {
    PROP_INFO_ADDONE(WatchInRange);
    var.removeFromDomain(val); 
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    D_ASSERT(v_size == 1);
    return (v[0] != val);
  }

  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }
  
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
    if(var.getMin() != val)
    {
        assignment.push_back(make_pair(0, var.getMin()));
        return true;
    }
    if(var.getMax() != val)
    {
        assignment.push_back(make_pair(0, var.getMax()));
        return true;
    }
    return false;
  }

   AbstractConstraint* reverse_constraint();
};

// For reverse constraint.
#include "dynamic_literal.h"

inline AbstractConstraint*
BuildCT_WATCHED_NOTLIT(const vector<BoolVarRef>& vec, const ConstraintBlob& b)
{ return new WatchNotLiteralBoolConstraint(vec[0], b.constants[0][0]); }

template<typename VarArray1>
AbstractConstraint*
BuildCT_WATCHED_NOTLIT(const VarArray1& _var_array_1, const ConstraintBlob& b)
{ 
  return new WatchNotLiteralConstraint<typename VarArray1::value_type>
    (_var_array_1[0], b.constants[0][0]); 
}

/* JSON
  { "type": "constraint",
    "name": "w-notliteral",
    "internal_name": "CT_WATCHED_NOTLIT",
    "args": [ "read_var", "read_constant" ]
  }
*/
#endif
