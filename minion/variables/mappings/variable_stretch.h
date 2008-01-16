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

template<typename T>
struct MultiplyHelp
{
static inline int round_down(int val, int divisor)
{
 // D_ASSERT(divisor > 0);
  if(val > 0)
    return val / divisor;
  
  int newval = val / divisor;
  if(newval * divisor == val)
    return newval;
  
  return newval - 1;
}

static inline int round_up(int val, int divisor)
{
  D_ASSERT(divisor > 0);
  if(val < 0)
    return val / divisor;
  
  int newval = val / divisor;
  if(newval*divisor == val)
    return newval;
  
  return newval + 1;
}

static inline int divide_exact(int val, int divisor)
{ 
  D_ASSERT(val % divisor == 0);
  return val / divisor;
}
};
/*
template<>
struct MultiplyHelp<BoolVarRef>
{
  static inline int round_down(int val, int divisor)
  {
    if(val < divisor)
      return 0;
    else
      return 1;
  }
  
  static inline int round_up(int val, int divisor)
  {
	if(
  }
  
};*/

template<typename VarRef>
struct MultiplyVar
{
  static const BOOL isBool = true;
  static const BoundType isBoundConst = Bound_Yes;
  BOOL isBound()
  { return true; }
  
  VarRef data;
  int Multiply;
  MultiplyVar(const VarRef& _data, int _Multiply) : data(_data), Multiply(_Multiply)
  { }
  
  MultiplyVar() : data()
  { Multiply = 0;}
  
  MultiplyVar(const MultiplyVar& b) : data(b.data), Multiply(b.Multiply)
  { }
  
  BOOL isAssigned()
  { return data.isAssigned(); }
  
  int getAssignedValue()
  { return data.getAssignedValue() * Multiply; }
  
  BOOL isAssignedValue(int i)
  { 
    if(data.isAssigned()) return false;
	
	return data.getAssignedValue() == i * Multiply;
  }
  
  BOOL inDomain(int b)
  { 
    if(b % Multiply != 0)
	  return false;
	return data.inDomain(MultiplyHelp<VarRef>::divide_exact(b, Multiply));
  }
  
  BOOL inDomain_noBoundCheck(int b)
  { 
    if(b % Multiply != 0)
	  return false;
	return data.inDomain(MultiplyHelp<VarRef>::divide_exact(b, Multiply));
  }
  
  int getMax()
  {  
    if(Multiply >= 0)
      return data.getMax() * Multiply; 
	else
	  return data.getMin() * Multiply;
  }
  
  int getMin()
  { 
    if(Multiply >= 0)
	  return data.getMin() * Multiply; 
	else
	  return data.getMax() * Multiply;  
  }

  int getInitialMax() const
  {  
    if(Multiply >= 0)
      return data.getInitialMax() * Multiply; 
	else
	  return data.getInitialMin() * Multiply;
  }
  
  int getInitialMin() const
  { 
    if(Multiply >= 0)
	  return data.getInitialMin() * Multiply; 
	else
	  return data.getInitialMax() * Multiply;  
  }
  
  void setMax(int i)
  { 
    if(Multiply >= 0)
      data.setMax(MultiplyHelp<VarRef>::round_down(i, Multiply)); 
	else
	  data.setMin(MultiplyHelp<VarRef>::round_up(-i, -Multiply));  
  }
  
  void setMin(int i)
  { 
    if(Multiply >= 0)
	  data.setMin(MultiplyHelp<VarRef>::round_up(i, Multiply));
	else
	  data.setMax(MultiplyHelp<VarRef>::round_down(-i, -Multiply));  
  }
  
  void uncheckedAssign(int b)
  { 
    D_ASSERT(b % Multiply == 0);
    data.uncheckedAssign(MultiplyHelp<VarRef>::divide_exact(b, Multiply)); 
  }
  
  void propogateAssign(int b)
  { data.propogateAssign(MultiplyHelp<VarRef>::divide_exact(b, Multiply)); }
  
  void removeFromDomain(int)
  { FAIL_EXIT(); }

  void addTrigger(Trigger t, TrigType type)
  { 
    switch(type)
	{
	  case UpperBound:
		if(Multiply>=0)
		  data.addTrigger(t, UpperBound);
		else
		  data.addTrigger(t, LowerBound);
		break;
	  case LowerBound:
		if(Multiply>=0)
		  data.addTrigger(t, LowerBound);
		else
		  data.addTrigger(t, UpperBound);
		break;
	  case Assigned:
	  case DomainChanged:
	    data.addTrigger(t, type);
	}
  }

#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, int pos = -999)
  {  data.addDynamicTrigger(t, type, pos); }
#endif

  
  operator string()
  { return "Mult " + to_string(Multiply) + ":" + string(data); }
  
  int getDomainChange(DomainDelta d)
  { return Multiply * data.getDomainChange(d); }
};

template<typename T>
struct MultiplyType
{ typedef MultiplyVar<T> type; };

template<typename T>
struct MultiplyType<vector<T> >
{ typedef vector<MultiplyVar<T> > type; };

template<typename T, std::size_t i>
struct MultiplyType<array<T, i> >
{ typedef array<MultiplyVar<T>, i> type; };


template<typename VRef>
typename MultiplyType<VRef>::type
MultiplyVarRef(VRef var_ref, int i)
{ return MultiplyVar<VRef>(var_ref, i); }

template<typename VarRef>
vector<MultiplyVar<VarRef> >
MultiplyVarRef(const vector<VarRef>& var_array, const vector<int>& multiplies)
{
  vector<MultiplyVar<VarRef> > Multiply_array(var_array.size());
  for(unsigned int i = 0; i < var_array.size(); ++i)
    Multiply_array[i] = MultiplyVarRef(var_array[i], multiplies[i]);
  return Multiply_array;
}

template<typename VarRef, std::size_t i>
array<MultiplyVar<VarRef>, i>
MultiplyVarRef(const array<VarRef, i>& var_array, const array<int, i>& multiplies)
{
  array<MultiplyVar<VarRef>, i> Multiply_array;
  for(unsigned int l = 0; l < i; ++l)
    Multiply_array[l] = MultiplyVarRef(var_array[l], multiplies[i]);
  return Multiply_array;
}

