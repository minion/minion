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

/** @help constraints;w-literal Description
  The constraint w-literal(x, a) ensures that x=a.
*/

/** @help constraints;w-literal References
  See also

  help constraints w-notliteral
*/

#ifndef CONSTRAINT_DYNAMIC_UNARY_LITERAL_H
#define CONSTRAINT_DYNAMIC_UNARY_LITERAL_H

#include "dynamic_notliteral.h"

// Checks if a variable is equal to a value.
template<typename Var>
  struct WatchLiteralConstraint : public AbstractConstraint
{
  virtual string constraint_name()
    { return "WatchedLiteral"; }

  Var var;

  DomainInt val;

  template<typename T>
  WatchLiteralConstraint(StateObj* _stateObj, const Var& _var, const T& _val) :
    AbstractConstraint(_stateObj), var(_var), val(_val) {}

  int dynamic_trigger_count()
  { return 0; }

  virtual void full_propagate()
  { var.propagateAssign(val); }


  virtual void propagate(DynamicTrigger* dt)
  {
    PROP_INFO_ADDONE(WatchInRange);
    var.propagateAssign(val);
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 1);
    return (v[0] == val);
  }

  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  { 
    if(var.inDomain(val))
    {
        assignment.push_back(make_pair(0, val));
        return true;
    }
    else
        return false;
  }

   virtual AbstractConstraint* reverse_constraint()
  { return new WatchNotLiteralConstraint<Var>(stateObj, var, val); }
};

// From dynamic_notliteral.h
template<typename Var>
  AbstractConstraint* WatchNotLiteralConstraint<Var>::reverse_constraint()
  { return new WatchLiteralConstraint<Var>(stateObj, var, val); }

  inline AbstractConstraint* WatchNotLiteralBoolConstraint::reverse_constraint()
  { return new WatchLiteralConstraint<BoolVarRef>(stateObj, var, val); }
#endif
