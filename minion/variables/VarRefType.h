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

// The follow two types are designed to allow turning a variable type which
// must be fed to a container, into a stand-alone class which is ready to be
// used as a variable.
template<typename InternalRefType>
struct VarRefType
{
  static const BOOL isBool = InternalRefType::isBool;
  static const BoundType isBoundConst = InternalRefType::isBoundConst;

  InternalRefType data;
  
  BOOL isBound() const
  { return data.isBound();}
  
  VarRefType(const InternalRefType& _data) : data(_data)
  {}
  
  VarRefType() : data()
  {}
  
  VarRefType(const VarRefType& b) : data(b.data)
  {}
  
  BOOL isAssigned() const
  { return GET_CONTAINER().isAssigned(data); }
  
  DomainInt getAssignedValue() const
  { return GET_CONTAINER().getAssignedValue(data); }
  
  BOOL isAssignedValue(DomainInt i) const
  { 
    return GET_CONTAINER().isAssigned(data) &&
    GET_CONTAINER().getAssignedValue(data) == i;
  }
  
  BOOL inDomain(DomainInt b) const
  { return GET_CONTAINER().inDomain(data, b); }

  BOOL inDomain_noBoundCheck(DomainInt b) const
  { return GET_CONTAINER().inDomain_noBoundCheck(data, b); }
  
  DomainInt getMax() const
  { return GET_CONTAINER().getMax(data); }
  
  DomainInt getMin() const
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
  
  void decisionAssign(DomainInt b)
  { GET_CONTAINER().decisionAssign(data, b); }
  
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
  
  BOOL isBound() const
  { return data.isBound();}
  
  QuickVarRefType(const InternalRefType& _data) : data(_data)
  {}
  
  QuickVarRefType() : data()
  {}
  
  QuickVarRefType(const QuickVarRefType& b) : data(b.data)
  {}
  
  BOOL isAssigned() const
  { return data.isAssigned(); }
  
  DomainInt getAssignedValue() const
  { return data.getAssignedValue(); }
  
  BOOL isAssignedValue(DomainInt i) const
  { 
    return data.isAssigned() &&
    data.getAssignedValue() == i;
  }
  BOOL inDomain(DomainInt b) const
  { return data.inDomain(b); }
  
  BOOL inDomain_noBoundCheck(DomainInt b) const
  { return data.inDomain_noBoundCheck(b); }

  DomainInt getMax() const
  { return data.getMax(); }
  
  DomainInt getMin() const
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
  
  void decisionAssign(DomainInt b)
  { GET_CONTAINER().decisionAssign(data, b); }
  
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

#endif

