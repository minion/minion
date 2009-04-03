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

#ifndef VARIABLE_NEG_H
#define VARIABLE_NEG_H

#include "../../constraints/constraint_abstract.h"

template<typename VarT>
struct VarNeg
{
  static const BOOL isBool = false;
  static const BoundType isBoundConst = VarT::isBoundConst;
  VarT data;

  BOOL isBound() const
  { return data.isBound();}
  
  VarNeg(VarT _data) : data(_data)
  {}
  
  VarNeg() : data()
  {}
  
  VarNeg(const VarNeg& b) : data(b.data)
  {}
  
  BOOL isAssigned() const
  { return data.isAssigned(); }
  
  DomainInt getAssignedValue() const
  { return -data.getAssignedValue(); }
  
  BOOL isAssignedValue(DomainInt i) const
  { 
    return data.isAssigned() &&
    data.getAssignedValue() == -i;
  }
  
  BOOL inDomain(DomainInt b) const
  { return data.inDomain(-b); }

  BOOL inDomain_noBoundCheck(DomainInt b) const
  { return data.inDomain(-b); }
  
  DomainInt getMax() const
  { return -data.getMin(); }
  
  DomainInt getMin() const
  { return -data.getMax(); }

  DomainInt getInitialMax() const
  { return -data.getInitialMin(); }
  
  DomainInt getInitialMin() const
  { return -data.getInitialMax(); }
  
  void setMax(DomainInt i)
  { data.setMin(-i); }
  
  void setMin(DomainInt i)
  { data.setMax(-i); }
  
  void uncheckedAssign(DomainInt b)
  { data.uncheckedAssign(-b); }
  
  void propagateAssign(DomainInt b)
  { data.propagateAssign(-b); }

  void decisionAssign(DomainInt b)
  { data.decisionAssign(-b); }

  void removeFromDomain(DomainInt b)
  { data.removeFromDomain(-b); }
  
  /// There isn't a minus sign here as domain changes from both the top and bottom of the domain are positive numbers.
  int getDomainChange(DomainDelta d)
  { return data.getDomainChange(d); }
  
 void addTrigger(Trigger t, TrigType type)
  { 
    switch(type)
	{
	  case UpperBound:
		data.addTrigger(t, LowerBound);
		break;
	  case LowerBound:
		data.addTrigger(t, UpperBound);
		break;
	  case Assigned:
	  case DomainChanged:
	    data.addTrigger(t, type);
      break;
	  default:
      D_FATAL_ERROR("Invalid trigger in 'neg' mapper");
	}
  }
    
  friend std::ostream& operator<<(std::ostream& o, const VarNeg& n)
  { return o << "Neg " << n.data; }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  
    switch(type)
    {
      case UpperBound:
        data.addDynamicTrigger(t, LowerBound);
        break;
      case LowerBound:
        data.addDynamicTrigger(t, UpperBound);
        break;
      case Assigned:
      case DomainChanged:
        data.addDynamicTrigger(t, type);
        break;
      case DomainRemoval:
        data.addDynamicTrigger(t, DomainRemoval, -pos); 
        break;
      default:
        D_FATAL_ERROR("Broken dynamic trigger");
    }
  }
#endif

  vector<AbstractConstraint*>* getConstraints()
  { return data.getConstraints(); }

  void addConstraint(AbstractConstraint* c)
  { data.addConstraint(c); }

  DomainInt getBaseVal(DomainInt v) const { return data.getBaseVal(-v); }

  Var getBaseVar() const { return data.getBaseVar(); }

#ifdef WDEG
  int getBaseWdeg()
  { return data.getBaseWdeg(); }

  void incWdeg()
  { data.incWdeg(); }
#endif
};



template<typename T>
struct NegType
{ typedef VarNeg<T> type; };


template<typename T>
struct NegType<vector<T> >
{ typedef vector<typename NegType<T>::type> type; };

#ifdef LIGHT_VECTOR
template<typename T>
struct NegType<vector<T> >
{ typedef vector<typename NegType<T>::type> type; };
#endif

template<typename T, std::size_t i>
struct NegType<array<T, i> >
{ typedef array<typename NegType<T>::type, i> type; };

// Neg of a neg is the original!
template<typename T>
struct NegType<VarNeg<T> >
{ typedef T type; };

template<typename VRef>
typename NegType<VRef>::type
VarNegRef(const VRef& var_ref)
{ return VarNeg<VRef>(var_ref); }

template<typename VRef>
VRef
VarNegRef(const VarNeg<VRef>& var_ref)
{ return var_ref.data; }

template<typename VarRef>
vector<typename NegType<VarRef>::type>
VarNegRef(const vector<VarRef>& var_array)
{
  vector<typename NegType<VarRef>::type> neg_array;
  neg_array.reserve(var_array.size());
  for(unsigned int i = 0; i < var_array.size(); ++i)
    neg_array.push_back(VarNegRef(var_array[i]));
  return neg_array;
}

#ifdef LIGHT_VECTOR
template<typename VarRef>
vector<typename NegType<VarRef>::type>
VarNegRef(const vector<VarRef>& var_array)
{
  vector<typename NegType<VarRef>::type> neg_array(var_array.size());
  for(unsigned int i = 0; i < var_array.size(); ++i)
    neg_array[i] = VarNegRef(var_array[i]);
  return neg_array;
}
#endif

template<typename VarRef, std::size_t i>
array<typename NegType<VarRef>::type, i>
VarNegRef(const array<VarRef, i>& var_array)
{
  array<typename NegType<VarRef>::type, i> neg_array;
  for(unsigned int l = 0; l < i; ++l)
    neg_array[l] = VarNegRef(var_array[l]);
  return neg_array;
}

#endif
