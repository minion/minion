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
  static const bool isBool = false;
  static const BoundType isBoundConst = Bound_Yes;
  
  bool isBound()
  { return true;}
  
  int val;
  explicit ConstantVar(int _val) : val(_val)
  {}
  
  ConstantVar() 
  {}
  
  ConstantVar(const ConstantVar& b) : val(b.val)
  {}
  
  bool isAssigned() const
  { return true;}
  
  int getAssignedValue() const
  { return val;}
  
  bool isAssignedValue(int i) const
  { return i == val; }
  
  bool inDomain(int b) const
  { return b == val; }

  bool inDomain_noBoundCheck(int b) const
  { 
    D_ASSERT(b == val);
	return true;
  }
  
  int getMax() const
  { return val; }
  
  int getMin() const
  { return val; }

  int getInitialMax() const
  { return val; }
  
  int getInitialMin() const
  { return val; }
  
  void setMax(int i)
  { if(i<val) Controller::fail(); }
  
  void setMin(int i)
  { if(i>val) Controller::fail(); }
  
  void uncheckedAssign(int)
  { FAIL_EXIT(); }
  
  void propogateAssign(int b)
  {if(b != val) Controller::fail(); }
  
  void removeFromDomain(int b)
  { if(b==val) Controller::fail(); }
 
  void addTrigger(Trigger, TrigType, int = -999)
  { }

  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* dt, TrigType, int = -999)
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
