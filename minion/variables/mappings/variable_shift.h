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


template<typename VarRef, typename ShiftType>
struct ShiftVar
{
  static const BOOL isBool = false;
  static const BoundType isBoundConst = VarRef::isBoundConst;
  VarRef data;
  
  BOOL isBound()
  { return data.isBound();}
  
  ShiftType shift;
  ShiftVar(const VarRef& _data, ShiftType _shift) : data(_data), shift(_shift)
  { }
  
  ShiftVar() : data(), shift()
  { }
  
  ShiftVar(const ShiftVar& b) : data(b.data), shift(b.shift)
  { }
  
  BOOL isAssigned()
  { return data.isAssigned(); }
  
  int getAssignedValue()
  { return data.getAssignedValue() + shift.val(); }
  
  BOOL isAssignedValue(int i)
  { return data.getAssignedValue() == i - shift.val(); }
  
  BOOL inDomain(int i)
  { return data.inDomain(i - shift.val()); }

  BOOL inDomain_noBoundCheck(int i)
  { return data.inDomain(i - shift.val()); }
  
  int getMax()
  { return data.getMax() + shift.val(); }
  
  int getMin()
  { return data.getMin() + shift.val(); }

  int getInitialMax() const
  { return data.getInitialMax() + shift.val(); }
  
  int getInitialMin() const
  { return data.getInitialMin() + shift.val(); }
  
  void setMax(int i)
  { data.setMax(i - shift.val()); }
  
  void setMin(int i)
  { data.setMin(i - shift.val()); }
  
  void uncheckedAssign(int b)
  { data.uncheckedAssign(b - shift.val()); }
  
  void propogateAssign(int b)
  { data.propogateAssign(b - shift.val()); }
  
  void removeFromDomain(int b)
  { data.removeFromDomain(b - shift.val()); }
    
 void addTrigger(Trigger t, TrigType type)
  { 
    switch(type)
	{
	  case UpperBound:
	  case LowerBound:
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
  { return "Shift " + to_string(shift.val()) + ":" + string(data); }
  
  int getDomainChange(DomainDelta d)
  { return data.getDomainChange(d); }
};

template<typename T, typename U>
struct ShiftType
{ typedef ShiftVar<T,U> type; };

template<typename T,typename U>
struct ShiftType<vector<T>, U>
{ typedef vector<ShiftVar<T, U> > type; };

#ifdef LIGHT_VECTOR
template<typename T,typename U>
struct ShiftType<light_vector<T>, U>
{ typedef light_vector<ShiftVar<T, U> > type; };
#endif

template<typename T, std::size_t i, typename U>
struct ShiftType<array<T, i>, U >
{ typedef array<ShiftVar<T, U>, i> type; };


template<typename VRef, typename Shift>
typename ShiftType<VRef, Shift>::type
ShiftVarRef(VRef var_ref, Shift shift)
{ return ShiftVar<VRef, Shift>(var_ref, shift); }


template<typename VarRef, typename Shift>
vector<ShiftVar<VarRef, Shift> >
ShiftVarRef(const vector<VarRef>& var_array, const Shift& shift)
{
  vector<ShiftVar<VarRef, Shift> > shift_array(var_array.size());
  for(unsigned int i = 0; i < var_array.size(); ++i)
    shift_array[i] = ShiftVarRef(var_array[i], shift);
  return shift_array;
}

#ifdef LIGHT_VECTOR
template<typename VarRef, typename Shift>
light_vector<ShiftVar<VarRef, Shift> >
ShiftVarRef(const light_vector<VarRef>& var_array, const Shift& shift)
{
  light_vector<ShiftVar<VarRef, Shift> > shift_array(var_array.size());
  for(unsigned int i = 0; i < var_array.size(); ++i)
    shift_array[i] = ShiftVarRef(var_array[i], shift);
  return shift_array;
}
#endif

template<typename VarRef, typename Shift, std::size_t i>
array<ShiftVar<VarRef, Shift>, i>
ShiftVarRef(const array<VarRef, i>& var_array, const Shift& shift)
{
  array<ShiftVar<VarRef, Shift>, i> shift_array;
  for(unsigned int l = 0; l < i; ++l)
    shift_array[l] = ShiftVarRef(var_array[l], shift);
  return shift_array;
}

