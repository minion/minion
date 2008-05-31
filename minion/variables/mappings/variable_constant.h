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

#include "../../CSPSpec.h"

#include "../../constraints/constraint_abstract.h"

struct ConstantVar
{
  // TODO: This really only needs enough to get 'fail'
  StateObj* stateObj;
  
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_Yes;
  
  // Hmm.. no sure if it's better to make this true or false.
  BOOL isBound() const
  { return true;}
  
  DomainInt val;
  
  explicit ConstantVar(StateObj* _stateObj, DomainInt _val) : stateObj(_stateObj), val(_val)
  {}
  
  ConstantVar() 
  {}
  
  ConstantVar(const ConstantVar& b) : stateObj(b.stateObj), val(b.val)
  {}
  
  BOOL isAssigned() const
  { return true;}
  
  DomainInt getAssignedValue() const
  { return val;}
  
  BOOL isAssignedValue(DomainInt i) const
  { return i == val; }
  
  BOOL inDomain(DomainInt b) const
  { return b == val; }

  BOOL inDomain_noBoundCheck(DomainInt b) const
  { 
    D_ASSERT(b == val);
	return true;
  }
  
  DomainInt getMax() const
  { return val; }
  
  DomainInt getMin() const
  { return val; }

  DomainInt getInitialMax() const
  { return val; }
  
  DomainInt getInitialMin() const
  { return val; }
  
  void setMax(DomainInt i)
  { if(i<val) getState(stateObj).setFailed(true); }
  
  void setMin(DomainInt i)
  { if(i>val) getState(stateObj).setFailed(true); }
  
  void uncheckedAssign(DomainInt)
  { FAIL_EXIT(); }
  
  void propagateAssign(DomainInt b)
  {if(b != val) getState(stateObj).setFailed(true); }
  
  void decisionAssign(DomainInt b)
  { propagateAssign(b); }
  
  void removeFromDomain(DomainInt b)
  { if(b==val) getState(stateObj).setFailed(true); }
 
  void addTrigger(Trigger, TrigType)
  { }

  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* dt, TrigType, DomainInt = -999)
  { dt->remove(); }
#endif

  vector<AbstractConstraint*>* getConstraints() { return NULL; }

  void addConstraint(AbstractConstraint* c){ ; }

  DomainInt getBaseVal(DomainInt v) const 
  { 
    D_ASSERT(v == val);
    return val; 
  }

  Var getBaseVar() const { return Var(VAR_CONSTANT, val); }

#ifdef WDEG
  int getBaseWdeg() { return 0; } //wdeg is irrelevant for non-search var

  void incWdeg() { ; }
#endif

  int getDomainChange(DomainDelta d)
  { 
    D_ASSERT(d.XXX_get_domain_diff() == 0);
	return 0;
  }
  
  friend std::ostream& operator<<(std::ostream& o, const ConstantVar& constant)
  { return o << "Constant" << constant.val; }
};

