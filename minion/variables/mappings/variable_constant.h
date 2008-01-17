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

struct ConstantVar
{
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_Yes;
  
  BOOL isBound()
  { return true;}
  
  DomainInt val;
  explicit ConstantVar(DomainInt _val) : val(_val)
  {}
  
  ConstantVar() 
  {}
  
  ConstantVar(const ConstantVar& b) : val(b.val)
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
  { if(i<val) Controller::fail(); }
  
  void setMin(DomainInt i)
  { if(i>val) Controller::fail(); }
  
  void uncheckedAssign(DomainInt)
  { FAIL_EXIT(); }
  
  void propogateAssign(DomainInt b)
  {if(b != val) Controller::fail(); }
  
  void removeFromDomain(DomainInt b)
  { if(b==val) Controller::fail(); }
 
  void addTrigger(Trigger, TrigType)
  { }

  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* dt, TrigType, DomainInt = -999)
  { dt->remove(); }
#endif

  int getDomainChange(DomainDelta d)
  { 
    D_ASSERT(d.XXX_get_domain_diff() == 0);
	return 0;
  }
  
  operator string()
  { return "Constant:" + to_string(val); }
};

