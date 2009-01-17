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
  {VAR_INFO_ADDONE(VAR_TYPE, construct);}
  
  InfoRefType(const InfoRefType& b) : data(b.data)
  {VAR_INFO_ADDONE(VAR_TYPE, copy);}
  
  BOOL isAssigned() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, isAssigned);
    return data.isAssigned(); 
  }
  
  DomainInt getAssignedValue() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getAssignedValue);
    return data.getAssignedValue(); }
  
  BOOL isAssignedValue(DomainInt i) const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, isAssignedValue);
    return data.isAssignedValue(i);
  }
  
  BOOL inDomain(DomainInt b) const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, inDomain);
    return data.inDomain( b); 
  }
  
  BOOL inDomain_noBoundCheck(DomainInt b) const
  {
    VAR_INFO_ADDONE(VAR_TYPE, inDomain_noBoundCheck);
    return data.inDomain_noBoundCheck(b);
  }
  
  
  DomainInt getMax() const
  {
    VAR_INFO_ADDONE(VAR_TYPE, getMax);
    return data.getMax(); 
  }
  
  DomainInt getMin() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getMin);
    return data.getMin(); 
  }

  DomainInt getInitialMax() const
  {
    VAR_INFO_ADDONE(VAR_TYPE, getInitialMax);
    return data.getInitialMax(); 
  }
  
  DomainInt getInitialMin() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getInitialMin);
    return data.getInitialMin(); 
  }
  
  void setMax(DomainInt i)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, setMax);
    data.setMax(i); 
  }
  
  void setMin(DomainInt i)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, setMin);
    data.setMin(i); 
  }
  
  void uncheckedAssign(DomainInt b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, uncheckedAssign);
    data.uncheckedAssign( b); 
  }
  
  void propagateAssign(DomainInt b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, propagateAssign);
    data.propagateAssign( b); 
  }
  
  void decisionAssign(DomainInt b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, decisionAssign);
    data.decisionAssign(b); 
  }
  
  void removeFromDomain(DomainInt b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, RemoveFromDomain);
    data.removeFromDomain( b); 
  }
  
  void addTrigger(Trigger t, TrigType type)
  {
    VAR_INFO_ADDONE(VAR_TYPE, addTrigger);
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


