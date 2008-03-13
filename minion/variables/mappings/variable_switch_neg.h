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

#include "../../constraints/constraint_abstract.h"

template<typename Var>
struct SwitchNeg
{
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Var::isBoundConst;
  Var data;
	
  BOOL isBound()
  { return data.isBound();}
  

  DomainInt multiplier;
  SwitchNeg(Var _data, int _multiplier) : data(_data), multiplier(_multiplier)
  { D_ASSERT(multiplier == -1 || multiplier == 1); }
  
  SwitchNeg() : data()
  {}
  
  SwitchNeg(const SwitchNeg& b) : data(b.data), multiplier(b.multiplier)
  {}
  
  BOOL isAssigned()
  { return data.isAssigned(); }
  
  DomainInt getAssignedValue()
  { return multiplier * data.getAssignedValue(); }
  
  BOOL isAssignedValue(DomainInt i)
  { 
	return data.isAssigned() &&
	data.getAssignedValue() == i * multiplier;
  }
  
  BOOL inDomain(DomainInt b)
  { return data.inDomain(b * multiplier); }
  
  BOOL inDomain_noBoundCheck(DomainInt b)
  { return data.inDomain(b * multiplier); }
  
  DomainInt getMax()
  { 
	if(multiplier == 1)
	  return data.getMax();
	else
	  return -data.getMin(); 
  }
  
  DomainInt getMin()
  { 
	if(multiplier == 1)
	  return data.getMin();
	else
	  return -data.getMax(); 
  }
  
  DomainInt getInitialMax() const
  { 
	if(multiplier == 1)
	  return data.getInitialMax();
	else
	  return -data.getInitialMin(); 
  }
  
  DomainInt getInitialMin() const
  { 
	if(multiplier == 1)
	  return data.getInitialMin();
	else
	  return -data.getInitialMax(); 
  }
  
  void setMax(DomainInt i)
  { 
	if(multiplier == 1)
	  data.setMax(i);
	else
	  data.setMin(-i); 
  }
  
  void setMin(DomainInt i)
  { 
	if(multiplier == 1)
	  data.setMin(i);
	else
	  data.setMax(-i); 
  }
  
  void uncheckedAssign(DomainInt b)
  { data.uncheckedAssign(b * multiplier); }
  
  void propagateAssign(DomainInt b)
  { data.propagateAssign(b * multiplier); }
  
  void removeFromDomain(DomainInt b)
  { data.removeFromDomain(b * multiplier); }
  
  /// There isn't a minus sign here as domain changes from both the top and bottom of the domain are positive numbers.
  int getDomainChange(DomainDelta d)
  { return data.getDomainChange(d); }

  void addTrigger(Trigger t, TrigType type)
  { 
    if(multiplier == 1)
	{
	  data.addTrigger(t, type);
	  return;
	}
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
	}
  }
  
 friend std::ostream& operator<<(std::ostream& o, const SwitchNeg& v)
 {
   return o << "SwitchNeg " << v.multiplier << ":" << v.data;
 }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  data.addDynamicTrigger(t, type, pos); }
#endif

  vector<AbstractConstraint*>* getConstraints()
  { return data.getConstraints(); }

  void addConstraint(AbstractConstraint* c)
  { data.addConstraint(c); }
};



template<typename T>
struct SwitchNegType
{ typedef SwitchNeg<T> type; };

template<typename T>
struct SwitchNegType<vector<T> >
{ typedef vector<SwitchNeg<T> > type; };

#ifdef LIGHT_VECTOR
template<typename T>
struct SwitchNegType<light_vector<T> >
{ typedef light_vector<SwitchNeg<T> > type; };
#endif

template<typename T, std::size_t i>
struct SwitchNegType<array<T, i> >
{ typedef array<SwitchNeg<T>, i> type; };


template<typename VRef>
typename SwitchNegType<VRef>::type
SwitchNegRef(const VRef& var_ref)
{ return SwitchNeg<VRef>(var_ref); }

template<typename VarRef>
vector<SwitchNeg<VarRef> >
SwitchNegRef(const vector<VarRef>& var_array)
{
  vector<SwitchNeg<VarRef> > neg_array;
  neg_array.reserve(var_array.size());
  for(unsigned int i = 0; i < var_array.size(); ++i)
	neg_array.push_back(SwitchNegRef(var_array[i]));
  return neg_array;
}

#ifdef LIGHT_VECTOR
template<typename VarRef>
light_vector<SwitchNeg<VarRef> >
SwitchNegRef(const light_vector<VarRef>& var_array)
{
  vector<SwitchNeg<VarRef> > neg_array(var_array.size);
  for(unsigned int i = 0; i < var_array.size(); ++i)
	neg_array[i] = SwitchNegRef(var_array[i]);
  return neg_array;
}
#endif

template<typename VarRef, std::size_t i>
array<SwitchNeg<VarRef>, i>
SwitchNegRef(const array<VarRef, i>& var_array)
{
  array<SwitchNeg<VarRef>, i> neg_array;
  for(unsigned int l = 0; l < i; ++l)
	neg_array[l] = SwitchNegRef(var_array[l]);
  return neg_array;
}

