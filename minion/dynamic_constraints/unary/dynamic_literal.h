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


  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
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
};

template<typename VarArray1>
AbstractConstraint*
WatchLiteralConDynamic(StateObj* stateObj, const VarArray1& _var_array_1, const ConstraintBlob& b)
{ 
  return new WatchLiteralConstraint<typename VarArray1::value_type>
    (stateObj, _var_array_1[0], b.constants[0][0]); 
}

BUILD_CONSTRAINT1_WITH_BLOB(CT_WATCHED_LIT, WatchLiteralConDynamic)
