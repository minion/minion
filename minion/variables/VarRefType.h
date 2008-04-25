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

#include "../constraints/constraint_abstract.h"

#ifndef VARREFTYPE_H
#define VARREFTYPE_H

#include "../CSPSpec.h"

// The follow three types are designed to allow turning a variable type which
// must be fed to a container, into a stand-alone class which is ready to be
// used as a variable.
template<typename InternalRefType>
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
  { return GET_CONTAINER().isAssigned(data); }
  
  DomainInt getAssignedValue()
  { return GET_CONTAINER().getAssignedValue(data); }
  
  BOOL isAssignedValue(DomainInt i)
  { 
    return GET_CONTAINER().isAssigned(data) &&
    GET_CONTAINER().getAssignedValue(data) == i;
  }
  
  BOOL inDomain(DomainInt b)
  { return GET_CONTAINER().inDomain(data, b); }

  BOOL inDomain_noBoundCheck(DomainInt b)
  { return GET_CONTAINER().inDomain_noBoundCheck(data, b); }
  
  DomainInt getMax()
  { return GET_CONTAINER().getMax(data); }
  
  DomainInt getMin()
  { return GET_CONTAINER().getMin(data); }

  DomainInt getInitialMax() const
  { return GET_CONTAINER().getInitialMax(data); }
  
  DomainInt getInitialMin() const
  { return GET_CONTAINER().getInitialMin(data); }
  
  void setMax(DomainInt i)
  { GET_CONTAINER().setMax(data,i); }
  
  void setMin(DomainInt i)
  { GET_CONTAINER().setMin(data,i); }
  
  void uncheckedAssign(DomainInt b)
  { GET_CONTAINER().uncheckedAssign(data, b); }
  
  void propagateAssign(DomainInt b)
  { GET_CONTAINER().propagateAssign(data, b); }
  
  void removeFromDomain(DomainInt b)
  { GET_CONTAINER().removeFromDomain(data, b); }
  
  void addTrigger(Trigger t, TrigType type)
  { GET_CONTAINER().addTrigger(data, t, type); }

  vector<AbstractConstraint*>* getConstraints()
  { return GET_CONTAINER().getConstraints(data); }

  void addConstraint(AbstractConstraint* c)
  { GET_CONTAINER().addConstraint(data, c); }

  DomainInt getBaseVal(DomainInt v) const
  { return GET_CONTAINER().getBaseVal(data, v); }

  Var getBaseVar() const
  { return GET_CONTAINER().getBaseVar(data); }

#ifdef WDEG
  int getBaseWdeg()
  { return GET_CONTAINER().getBaseWdeg(data); }

  void incWdeg()
  { GET_CONTAINER().incWdeg(data); }
#endif

  friend std::ostream& operator<<(std::ostream& o, const VarRefType& v)
  { return o << InternalRefType::name() << v.data.var_num; }
    
  int getDomainChange(DomainDelta d)
  { return d.XXX_get_domain_diff(); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  GET_CONTAINER().addDynamicTrigger(data, t, type, pos); }
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
  { GET_CONTAINER().setMax(data,i); }
  
  void setMin(DomainInt i)
  { GET_CONTAINER().setMin(data,i); }
  
  void uncheckedAssign(DomainInt b)
  { GET_CONTAINER().uncheckedAssign(data, b); }
  
  void propagateAssign(DomainInt b)
  { GET_CONTAINER().propagateAssign(data, b); }
  
  void removeFromDomain(DomainInt b)
  { GET_CONTAINER().removeFromDomain(data, b); }
  
  void addTrigger(Trigger t, TrigType type)
  { GET_CONTAINER().addTrigger(data, t, type); }

  vector<AbstractConstraint*>* getConstraints()
  { return GET_CONTAINER().getConstraints(data); }

  void addConstraint(AbstractConstraint* c)
  { GET_CONTAINER().addConstraint(data, c); }

  DomainInt getBaseVal(DomainInt v) const
  { return data.getBaseVal(v); }

  Var getBaseVar() const
  { return data.getBaseVar(); }

#ifdef WDEG
  int getBaseWdeg()
  { return GET_CONTAINER().getBaseWdeg(data); }

  void incWdeg()
  { GET_CONTAINER().incWdeg(data); }
#endif

  friend std::ostream& operator<<(std::ostream& o, const QuickVarRefType& b)
  { return o << "Bool:" << b.data; }
  
  int getDomainChange(DomainDelta d)
  { return d.XXX_get_domain_diff(); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  GET_CONTAINER().addDynamicTrigger(data, t, type, pos); }
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
  
  void propagateAssign(DomainInt b)
  { (data.getCon()).propagateAssign(data, b); }
  
  void removeFromDomain(DomainInt b)
  { (data.getCon()).removeFromDomain(data, b); }
  
  void addTrigger(Trigger t, TrigType type)
  { (data.getCon()).addTrigger(data, t, type); }

  vector<AbstractConstraint*>* getConstraints()
  { return (data.getCon()).getConstraints(data); }
  
  void addConstraint(AbstractConstraint* c)
  { (data.getCon()).addConstraint(data, c); }

  DomainInt getBaseVal(DomainInt v) const
  { return (data.getCon()).getBaseVal(v); }

  Var getBaseVar() const
  { return (data.getCon()).getBaseVar(); }

#ifdef WDEG
  int getBaseWdeg()
  { return (data.getCon()).getBaseWdeg(data); }

  void incWdeg()
  { (data.getCon()).incWdeg(data); }
#endif
  
  friend std::ostream& operator<<(std::ostream& o, const CompleteVarRefType& cv)
  { return o << "CompleteCon:" << cv.data.var_num; }
  
  int getDomainChange(DomainDelta d)
  { return d.XXX_get_domain_diff(); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  (data.getCon()).addDynamicTrigger(data, t, type, pos); }
#endif
};



#endif

