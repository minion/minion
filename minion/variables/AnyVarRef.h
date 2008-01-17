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


/// Internal type used by AnyVarRef.
struct AnyVarRef_Abstract
{
  virtual BOOL isBound() = 0;
  virtual BOOL isAssigned() = 0;  
  virtual DomainInt getAssignedValue() = 0;
  virtual BOOL isAssignedValue(DomainInt i) = 0;
  virtual BOOL inDomain(DomainInt b) = 0;
  virtual BOOL inDomain_noBoundCheck(DomainInt b) = 0;
  virtual DomainInt getMax() = 0;
  virtual DomainInt getMin() = 0;
  virtual DomainInt getInitialMax() const = 0;
  virtual DomainInt getInitialMin() const = 0;
  virtual void setMax(DomainInt i) = 0;
  virtual void setMin(DomainInt i) = 0;
  virtual void uncheckedAssign(DomainInt b) = 0;
  virtual void propogateAssign(DomainInt b) = 0;
  virtual void removeFromDomain(DomainInt b) = 0;
  virtual void addTrigger(Trigger t, TrigType type) = 0;

  virtual string virtual_to_string() = 0;
  
  virtual ~AnyVarRef_Abstract()
  {}
  
  virtual int getDomainChange(DomainDelta d) = 0;
#ifdef DYNAMICTRIGGERS
  virtual void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999) = 0;
#endif
};

/// Internal type used by AnyVarRef.
template<typename VarRef>
struct AnyVarRef_Concrete : public AnyVarRef_Abstract
{

  virtual BOOL isBound()
  { return data.isBound();}
  
  VarRef data;
  AnyVarRef_Concrete(const VarRef& _data) : data(_data)
  {}
  
  AnyVarRef_Concrete() 
  {}
  
  AnyVarRef_Concrete(const AnyVarRef_Concrete& b) : data(b.data)
  {}
  
  virtual BOOL isAssigned()
  { return data.isAssigned(); }
  
  virtual DomainInt getAssignedValue()
  { return data.getAssignedValue(); }
  
  virtual BOOL isAssignedValue(DomainInt i)
  { return data.isAssignedValue(i); }
  
  virtual BOOL inDomain(DomainInt b)
  { return data.inDomain(b); }
  
  virtual BOOL inDomain_noBoundCheck(DomainInt b)
  { return data.inDomain_noBoundCheck(b); }
  
  virtual DomainInt getMax()
  { return data.getMax(); }
  
  virtual DomainInt getMin()
  { return data.getMin(); }

  virtual DomainInt getInitialMax() const
  { return data.getInitialMax(); }
  
  virtual DomainInt getInitialMin() const
  { return data.getInitialMin(); }
  
  virtual void setMax(DomainInt i)
  { data.setMax(i); }
  
  virtual void setMin(DomainInt i)
  { data.setMin(i); }
  
  virtual void uncheckedAssign(DomainInt b)
  { data.uncheckedAssign(b); }
  
  virtual void propogateAssign(DomainInt b)
  { data.propogateAssign(b); }
  
  virtual void removeFromDomain(DomainInt b)
  { data.removeFromDomain(b); }
  
  virtual void addTrigger(Trigger t, TrigType type)
  { data.addTrigger(t, type); }
  
  virtual string virtual_to_string()
  { return string(data); }
  
  virtual ~AnyVarRef_Concrete()
  {}
  
  int getDomainChange(DomainDelta d)
  { return data.getDomainChange(d); }

#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  data.addDynamicTrigger(t, type, pos); }
#endif
};

/// Provides a method of wrapping any variable type in a general wrapper.
struct AnyVarRef
{
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_Maybe;
  shared_ptr<AnyVarRef_Abstract> data;
  
  BOOL isBound()
  { return data->isBound();}
  
  template<typename VarRef>
    AnyVarRef(const VarRef& _data) 
  { data = shared_ptr<AnyVarRef_Abstract>(new AnyVarRef_Concrete<VarRef>(_data)); }
  
  AnyVarRef() 
  {}
  
  AnyVarRef(const AnyVarRef& b) : data(b.data)
  {}
  
  BOOL isAssigned()
  { return data->isAssigned(); }
  
  DomainInt getAssignedValue()
  { return data->getAssignedValue(); }
  
  BOOL isAssignedValue(DomainInt i)
  { 
    return data->isAssigned() &&
    data->getAssignedValue() == i;
  }
  
  BOOL inDomain(DomainInt b)
  { return data->inDomain(b); }

  BOOL inDomain_noBoundCheck(DomainInt b)
  { return data->inDomain_noBoundCheck(b); }
  
  DomainInt getMax()
  { return data->getMax(); }
  
  DomainInt getMin()
  { return data->getMin(); }

  DomainInt getInitialMax() const
  { return data->getInitialMax(); }
  
  DomainInt getInitialMin() const
  { return data->getInitialMin(); }
  
  void setMax(DomainInt i)
  { data->setMax(i); }
  
  void setMin(DomainInt i)
  { data->setMin(i); }
  
  void uncheckedAssign(DomainInt b)
  { data->uncheckedAssign(b); }
  
  void propogateAssign(DomainInt b)
  { data->propogateAssign(b); }
  
  void removeFromDomain(DomainInt b)
  { data->removeFromDomain(b); }

  void addTrigger(Trigger t, TrigType type)
  { data->addTrigger(t, type); }
  
  operator string()
  { return "AnyVRef:" + data->virtual_to_string(); }
  
  int getDomainChange(DomainDelta d)
  { return data->getDomainChange(d); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  data->addDynamicTrigger(t, type, pos); }
#endif
};


