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
  struct WatchNotLiteralConstraint : public AbstractConstraint
{
  virtual string constraint_name() { return "WatchedNotLiteral"; }

  Var var;

  DomainInt val;

  template<typename T>
  WatchNotLiteralConstraint(StateObj* _stateObj, const Var& _var, const T& _val) :
    AbstractConstraint(_stateObj), var(_var), val(_val) {}

  int dynamic_trigger_count()
  { return 0; }

  virtual void full_propagate()
  { 
    var.removeFromDomain(val); 
  }


  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
    PROP_INFO_ADDONE(WatchInRange);
    var.removeFromDomain(val); 
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
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

  virtual void get_satisfying_assignment(box<pair<int,int> >& assignment)
  { 
    D_ASSERT(var.inDomain(var.getMin()) && var.inDomain(var.getMax()));
    DomainInt tmp;
    if((tmp = var.getMin()) != val)
	assignment.push_back(make_pair(0, tmp)); 
    else if((tmp = var.getMax()) != val)
	assignment.push_back(make_pair(0, tmp)); 
  }
};

struct WatchNotLiteralBoolConstraint : public AbstractConstraint
{
  virtual string constraint_name() { return "WatchedNotLiteral"; }

  BoolVarRef var;

  DomainInt val;

  template<typename T>
  WatchNotLiteralBoolConstraint(StateObj* _stateObj, const BoolVarRef& _var, const T& _val) :
    AbstractConstraint(_stateObj), var(_var), val(_val) 
  {
    cout << "using boolean specialisation" << endl; 
  }  

  int dynamic_trigger_count()
  { return 0; }

  virtual void full_propagate()
  { 
    var.removeFromDomain(val); 
  }

  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
    PROP_INFO_ADDONE(WatchInRange);
    var.removeFromDomain(val); 
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
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
  
  virtual void get_satisfying_assignment(box<pair<int,int> >& assignment)
  {
    if(var.inDomain(1 - val))
      assignment.push_back(make_pair(0, 1 - val));
  }
};

AbstractConstraint*
WatchNotLiteralConDynamic(StateObj* stateObj, const light_vector<BoolVarRef>& vec, const ConstraintBlob& b)
{ return new WatchNotLiteralBoolConstraint(stateObj, vec[0], b.constants[0][0]); }

template<typename VarArray1>
AbstractConstraint*
WatchNotLiteralConDynamic(StateObj* stateObj, const VarArray1& _var_array_1, const ConstraintBlob& b)
{ 
  return new WatchNotLiteralConstraint<typename VarArray1::value_type>
    (stateObj, _var_array_1[0], b.constants[0][0]); 
}

BUILD_CONSTRAINT1_WITH_BLOB(CT_WATCHED_NOTLIT, WatchNotLiteralConDynamic)
