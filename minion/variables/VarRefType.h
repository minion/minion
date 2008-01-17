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

#ifndef VARREFTYPE_H
#define VARREFTYPE_H


// The follow three types are designed to allow turning a variable type which
// must be fed to a container, into a stand-alone class which is ready to be
// used as a variable.
template<typename GetContainer, typename InternalRefType>
struct VarRefType
{
  static const BOOL isBool = InternalRefType::isBool;
  static const BoundType isBoundConst = InternalRefType::isBoundConst;
  InternalRefType data;
  
  BOOL isBound()
  { return data.isBound();}
  
  VarRefType(const InternalRefType& _data) : data(_data)
  {}
  
  VarRefType() : data()
  {}
  
  VarRefType(const VarRefType& b) : data(b.data)
  {}
  
  BOOL isAssigned()
  { return GetContainer::con().isAssigned(data); }
  
  DomainInt getAssignedValue()
  { return GetContainer::con().getAssignedValue(data); }
  
  BOOL isAssignedValue(DomainInt i)
  { 
    return GetContainer::con().isAssigned(data) &&
    GetContainer::con().getAssignedValue(data) == i;
  }
  
  BOOL inDomain(DomainInt b)
  { return GetContainer::con().inDomain(data, b); }

  BOOL inDomain_noBoundCheck(DomainInt b)
  { return GetContainer::con().inDomain_noBoundCheck(data, b); }
  
  DomainInt getMax()
  { return GetContainer::con().getMax(data); }
  
  DomainInt getMin()
  { return GetContainer::con().getMin(data); }

  DomainInt getInitialMax() const
  { return GetContainer::con().getInitialMax(data); }
  
  DomainInt getInitialMin() const
  { return GetContainer::con().getInitialMin(data); }
  
  void setMax(DomainInt i)
  { GetContainer::con().setMax(data,i); }
  
  void setMin(DomainInt i)
  { GetContainer::con().setMin(data,i); }
  
  void uncheckedAssign(DomainInt b)
  { GetContainer::con().uncheckedAssign(data, b); }
  
  void propogateAssign(DomainInt b)
  { GetContainer::con().propogateAssign(data, b); }
  
  void removeFromDomain(DomainInt b)
  { GetContainer::con().removeFromDomain(data, b); }
  
  void addTrigger(Trigger t, TrigType type)
  { GetContainer::con().addTrigger(data, t, type); }

  operator string()
  { return GetContainer::name() + to_string(data.var_num); }
  
  int getDomainChange(DomainDelta d)
  { return d.XXX_get_domain_diff(); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  GetContainer::con().addDynamicTrigger(data, t, type, pos); }
#endif
};


template<typename GetContainer, typename InternalRefType>
struct QuickVarRefType
{
  static const BOOL isBool = InternalRefType::isBool;
  static const BoundType isBoundConst = InternalRefType::isBoundConst;
  InternalRefType data;
  
  BOOL isBound()
  { return data.isBound();}
  
  QuickVarRefType(const InternalRefType& _data) : data(_data)
  {}
  
  QuickVarRefType() : data()
  {}
  
  QuickVarRefType(const QuickVarRefType& b) : data(b.data)
  {}
  
  BOOL isAssigned()
  { return data.isAssigned(); }
  
  DomainInt getAssignedValue()
  { return data.getAssignedValue(); }
  
  BOOL isAssignedValue(DomainInt i)
  { 
    return data.isAssigned() &&
    data.getAssignedValue() == i;
  }
  BOOL inDomain(DomainInt b)
  { return data.inDomain(b); }
  
  BOOL inDomain_noBoundCheck(DomainInt b)
  { return data.inDomain_noBoundCheck(b); }

  DomainInt getMax()
  { return data.getMax(); }
  
  DomainInt getMin()
  { return data.getMin(); }

  DomainInt getInitialMax() const
  { return data.getInitialMax(); }
  
  DomainInt getInitialMin() const
  { return data.getInitialMin(); }
  
  void setMax(DomainInt i)
  { GetContainer::con().setMax(data,i); }
  
  void setMin(DomainInt i)
  { GetContainer::con().setMin(data,i); }
  
  void uncheckedAssign(DomainInt b)
  { GetContainer::con().uncheckedAssign(data, b); }
  
  void propogateAssign(DomainInt b)
  { GetContainer::con().propogateAssign(data, b); }
  
  void removeFromDomain(DomainInt b)
  { GetContainer::con().removeFromDomain(data, b); }
  
  void addTrigger(Trigger t, TrigType type)
  { GetContainer::con().addTrigger(data, t, type); }
  
  operator string()
  { return "Bool:" + to_string(data.var_num); }
  
  int getDomainChange(DomainDelta d)
  { return d.XXX_get_domain_diff(); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  GetContainer::con().addDynamicTrigger(data, t, type, pos); }
#endif
};


template<typename InternalRefType>
struct CompleteVarRefType
{
  InternalRefType data;
  CompleteVarRefType(const InternalRefType& _data) : data(_data)
  {}
  
  CompleteVarRefType() 
  {}
  
  CompleteVarRefType(const CompleteVarRefType& b) : data(b.data)
  {}
  
  BOOL isAssigned()
  { return (data.getCon()).isAssigned(data); }
  
  DomainInt getAssignedValue()
  { return (data.getCon()).getAssignedValue(data); }
  
  BOOL isAssignedValue(DomainInt i)
  { 
    return (data.getCon()).isAssigned(data) &&
    (data.getCon()).getAssignedValue(data) == i;
  }
  BOOL inDomain(DomainInt b)
  { return (data.getCon()).inDomain(data, b); }
  
  DomainInt getMax()
  { return (data.getCon()).getMax(data); }
  
  DomainInt getMin()
  { return (data.getCon()).getMin(data); }

  DomainInt getInitialMax() const
  { return (data.getCon()).getInitialMax(data); }
  
  DomainInt getInitialMin() const
  { return (data.getCon()).getInitialMin(data); }
  
  void setMax(DomainInt i)
  { (data.getCon()).setMax(data,i); }
  
  void setMin(DomainInt i)
  { (data.getCon()).setMin(data,i); }
  
  void uncheckedAssign(DomainInt b)
  { (data.getCon()).uncheckedAssign(data, b); }
  
  void propogateAssign(DomainInt b)
  { (data.getCon()).propogateAssign(data, b); }
  
  void removeFromDomain(DomainInt b)
  { (data.getCon()).removeFromDomain(data, b); }
  
  void addTrigger(Trigger t, TrigType type)
  { (data.getCon()).addTrigger(data, t, type); }
  
  operator string()
  { return "CompleteCon:" + to_string(data.var_num); }
  
  int getDomainChange(DomainDelta d)
  { return d.XXX_get_domain_diff(); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  (data.getCon()).addDynamicTrigger(data, t, type, pos); }
#endif
};



#endif

