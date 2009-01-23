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

#include "../constraints/constraint_abstract.h"


/*
inline string var_name(const Var& var, CSPInstance& csp)
 { 
   if(var.type() == VAR_CONSTANT)
     print_instance( var.pos());
   else if(var.type() == VAR_NOTBOOL)
   {
     oss << "!";
     oss << csp.vars.getName(Var(VAR_BOOL, var.pos())); 
   }
   else
     oss << csp.vars.getName(var); 
 }
*/

#define VAR_INFO_PRINT_0(X,Y)
#define VAR_INFO_PRINT_1(X,Y,Z)

template<typename WrapType, Info_VarType VAR_TYPE>
struct InfoRefType
{
  WrapType data;
  
  static const BOOL isBool = WrapType::isBool;
  static const BoundType isBoundConst = WrapType::isBoundConst;

  BOOL isBound() const
  { return data.isBound();}
  
  InfoRefType(const WrapType& _data) : data(_data)
  { VAR_INFO_ADDONE(VAR_TYPE, copy); }
  
  InfoRefType() 
  { VAR_INFO_ADDONE(VAR_TYPE, construct); }
  
  InfoRefType(const InfoRefType& b) : data(b.data)
  { VAR_INFO_ADDONE(VAR_TYPE, copy); }
  
  bool isAssigned() const
  {
    VAR_INFO_ADDONE(VAR_TYPE, isAssigned);
    bool assign = data.isAssigned();
    VAR_INFO_PRINT_0("Assigned", assign);
    return assign;
  }
  
  DomainInt getAssignedValue() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getAssignedValue);
    DomainInt assignValue = data.getAssignedValue();
    VAR_INFO_PRINT_0("isAssignedValue", assignValue);
    return assignValue; 
  }
  
  bool isAssignedValue(DomainInt i) const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, isAssignedValue);
    bool isAssignValue = data.isAssignedValue(i);
    VAR_INFO_PRINT_1("is assigned ", i, isAssignValue);
    return isAssignValue;
  }
  
  bool inDomain(DomainInt b) const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, inDomain);
    bool indom = data.inDomain(b);
    VAR_INFO_PRINT_1("in domain", b, indom);
    return indom; 
  }
  
  bool inDomain_noBoundCheck(DomainInt b) const
  {
    VAR_INFO_ADDONE(VAR_TYPE, inDomain_noBoundCheck);
    bool indom_noBC = data.inDomain_noBoundCheck(b);
    VAR_INFO_PRINT_1("in domain, no bound check", b, indom_noBC);
    return indom_noBC;
  }
  
  
  DomainInt getMax() const
  {
    VAR_INFO_ADDONE(VAR_TYPE, getMax);
    DomainInt maxval = data.getMax();
    VAR_INFO_PRINT_0("GetMax", maxval);
    return maxval; 
  }
  
  DomainInt getMin() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getMin);
    DomainInt minval = data.getMin();
    VAR_INFO_PRINT_0("GetMin", minval);
    return minval; 
  }

  DomainInt getInitialMax() const
  {
    VAR_INFO_ADDONE(VAR_TYPE, getInitialMax);
    DomainInt initialMax = data.getInitialMax();
    VAR_INFO_PRINT_0("InitialMax", initialMax);
    return initialMax;
  }
  
  DomainInt getInitialMin() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getInitialMin);
    DomainInt initialMin = data.getInitialMin();
    VAR_INFO_PRINT_0("InitialMin", initialMin);
    return initialMin; 
  }
  
  void setMax(DomainInt i)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, setMax);
    VAR_INFO_PRINT_0("SetMax", i);
    data.setMax(i); 
  }
  
  void setMin(DomainInt i)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, setMin);
    VAR_INFO_PRINT_0("SetMin", i);
    data.setMin(i); 
  }
  
  void uncheckedAssign(DomainInt b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, uncheckedAssign);
    VAR_INFO_PRINT_0("uncheckedAssign", b);
    data.uncheckedAssign( b); 
  }
  
  void propagateAssign(DomainInt b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, propagateAssign);
    VAR_INFO_PRINT_0("propagateAssign", b);
    data.propagateAssign( b); 
  }
  
  void decisionAssign(DomainInt b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, decisionAssign);
    VAR_INFO_PRINT_0("decisionAssign", b);
    data.decisionAssign(b); 
  }
  
  void removeFromDomain(DomainInt b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, RemoveFromDomain);
    VAR_INFO_PRINT_0("removeFromDomain", b);
    data.removeFromDomain( b); 
  }
  
  void addTrigger(Trigger t, TrigType type)
  {
    VAR_INFO_ADDONE(VAR_TYPE, addTrigger);
    VAR_INFO_PRINT_0("addTrigger", type);
    data.addTrigger( t, type); 
  }

  vector<AbstractConstraint*>* getConstraints()
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getConstraints);
    return data.getConstraints();
  }
  
  void addConstraint(AbstractConstraint* c)
  {
    VAR_INFO_ADDONE(VAR_TYPE, addConstraint);
    data.addConstraint(c);
  }

  DomainInt getBaseVal(DomainInt v) const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getBaseVal);
    return data.getBaseVal(v);
  }

  Var getBaseVar() const 
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getBaseVar);
    return data.getBaseVar(); 
  }

#ifdef WDEG
  int getBaseWdeg()
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getBaseWdeg);
    return data.getBaseWdeg(); 
  }

  void incWdeg()
  { 
    VAR_INFO_ADDONE(VAR_TYPE, incWdeg);
    data.incWdeg(); 
  }
#endif
  
  friend std::ostream& operator<<(std::ostream& o, const InfoRefType& ir)
  {
    return o << "InfoRef " << ir.data;
  }
 
  int getDomainChange(DomainDelta d)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getDomainChange);
    return d.XXX_get_domain_diff(); 
  }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, addDynamicTrigger);
    data.addDynamicTrigger( t, type, pos); 
  }
#endif
};


