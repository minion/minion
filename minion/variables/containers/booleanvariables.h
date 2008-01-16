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




/// Standard data type used for storing compressed booleans 
typedef unsigned long data_type;
static const data_type one = 1;
static const data_type max_data = one << ( sizeof(data_type) - 1 );

struct BooleanContainer;

/// A reference to a boolean variable
struct BoolVarRef_internal
{
  static const BOOL isBool = true;
  static const BoundType isBoundConst = Bound_No;
  
  BOOL isBound()
  { return false;}
  
  data_type shift_offset;
  unsigned data_offset;
  unsigned var_num;
  VirtualBackTrackOffset data_position;
  VirtualMemOffset value_position;
  
  BoolVarRef_internal(int value, BooleanContainer* b_con);
  
  BoolVarRef_internal(const BoolVarRef_internal& b) :
    shift_offset(b.shift_offset), data_offset(b.data_offset), var_num(b.var_num), data_position(b.data_position),
    value_position(b.value_position)
  {}
  
  BoolVarRef_internal()
  { D_DATA(shift_offset = data_offset = var_num = ~1); }
  
  data_type& assign_ptr() const
  { return *static_cast<data_type*>(data_position.get_ptr()); }
  
  data_type& value_ptr() const
  { return *static_cast<data_type*>(value_position.get_ptr()); }
  
  BOOL isAssigned() const
  { return assign_ptr() & shift_offset; }
  
  int getAssignedValue() const
  {
    D_ASSERT(isAssigned());
    return (bool)(value_ptr() & shift_offset);
  }
  
  BOOL inDomain(int b) const
  {
    if(b < 0 || b > 1) 
	  return false;
    return (!isAssigned()) || (b == getAssignedValue());
  }
  
  BOOL inDomain_noBoundCheck(int b) const
  {
    D_ASSERT(b == 0 || b == 1);
	return (!isAssigned()) || (b == getAssignedValue());
  }
  
  int getMin() const
  {
    if(!isAssigned()) return 0;
    return getAssignedValue();
  }
  
  int getMax() const
  {
    if(!isAssigned()) return 1;
    return getAssignedValue();
  }
 
  int getInitialMin() const
  { return 0; }
  
  int getInitialMax() const
  { return 1; }
 
};

struct GetBooleanContainer;
typedef QuickVarRefType<GetBooleanContainer, BoolVarRef_internal> BoolVarRef;

/// Container for boolean variables
struct BooleanContainer
{
  static const int width = 7;
  BackTrackOffset assign_offset;
  MemOffset values_mem;
  unsigned var_count_m;
  TriggerList trigger_list;
  /// When false, no variable can be altered. When true, no variables can be created.
  BOOL lock_m;
  
  data_type* value_ptr()
  { return static_cast<data_type*>(values_mem.get_ptr()); }
  
  const data_type* value_ptr() const
  { return static_cast<const data_type*>(values_mem.get_ptr()); }
  
  data_type* assign_ptr()
  { return static_cast<data_type*>(assign_offset.get_ptr()); }
  
  const data_type* assign_ptr() const
  { return static_cast<const data_type*>(assign_offset.get_ptr()); }
  
  void lock()
  { 
    D_ASSERT(!lock_m);
    lock_m = true;
	int required_mem = var_count_m / 8 + 1;
	required_mem += (required_mem % sizeof(data_type));
    assign_offset.request_bytes(required_mem);
    values_mem.request_bytes(required_mem);
	// Min domain value = 0, max domain val = 1.
    trigger_list.lock(var_count_m, 0, 1);
  }
  
  BooleanContainer() :  var_count_m(0), lock_m(false)
  {}
  
  /// Returns a new Boolean Variable.
  BoolVarRef get_new_var();
  
  /// Returns a reference to the ith Boolean variable which was previously created.
  BoolVarRef get_var_num(int i);
  
  
  void setMax(const BoolVarRef_internal& d, int i) 
  {
    if(i < 0)
	{
	  Controller::fail();
	  return;
	}

    D_ASSERT(i >= 0);
	if(i==0)
      propogateAssign(d,0);
  }
  
  void setMin(const BoolVarRef_internal& d, int i) 
  {
    if(i > 1)
	{
	  Controller::fail();
	  return;
	}
    D_ASSERT(i <= 1);
    if(i==1)
      propogateAssign(d,1);
  }
  
  void removeFromDomain(const BoolVarRef_internal& d, int b)
  {
    D_ASSERT(lock_m && d.var_num < var_count_m);
    if(d.isAssigned())
    {
      if(b == d.getAssignedValue()) 
	    Controller::fail();
    }
    else
      uncheckedAssign(d,1-b);
  }
  
  void uncheckedAssign(const BoolVarRef_internal& d, int b)
  {
    D_ASSERT(lock_m && d.var_num < var_count_m);
    D_ASSERT(!d.isAssigned());
	if(b!=0 && b!=1)
    {
	  Controller::fail();
	  return;
	}
    assign_ptr()[d.data_offset] |= d.shift_offset;
    trigger_list.push_domain(d.var_num);
    trigger_list.push_assign(d.var_num, b);
	trigger_list.push_domain_removal(d.var_num, 1 - b);
    
    if(b)
    {
      trigger_list.push_lower(d.var_num, 1);
      value_ptr()[d.data_offset] |= d.shift_offset;
    }
    else
    {
      trigger_list.push_upper(d.var_num, 1);
      value_ptr()[d.data_offset] &= ~d.shift_offset;
    }
  }
  
  void propogateAssign(const BoolVarRef_internal& d, int b)
  {
    if(!d.isAssigned()) 
      uncheckedAssign(d,b);
    else
    {
      if(d.getAssignedValue() != b)
	Controller::fail();
    }
  }

  void addTrigger(BoolVarRef_internal& b, Trigger t, TrigType type)
  { D_ASSERT(lock_m); trigger_list.add_trigger(b.var_num, t, type); }
    
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(BoolVarRef_internal& b, DynamicTrigger* t, TrigType type, int pos = -999)
  { 
    D_ASSERT(pos == -999 || ( type == DomainRemoval && pos != -999 ) );
    D_ASSERT(lock_m);
	trigger_list.addDynamicTrigger(b.var_num, t, type, pos); 
  }
#endif
  
  /*  operator std::string()
  {
    D_ASSERT(lock_m);
    stringstream s;
    int char_count = 0;
    for(unsigned int i=0;i<var_count_m;i++)
    {
      if(!isAssigned(BoolVarRef_internal(i,this)))
	s << "X";
      else
      {
	s << (getAssignedValue(BoolVarRef_internal(i,this))?1:0); 
      }
      char_count++;
      if(char_count%width==0) s << endl;
    }
    return s.str();
  }*/
};

VARDEF(BooleanContainer boolean_container);

struct GetBooleanContainer
{ static BooleanContainer& con() { return boolean_container; } };




inline BoolVarRef BooleanContainer::get_new_var()
{ 
  D_ASSERT(!lock_m);
  return BoolVarRef(BoolVarRef_internal(var_count_m++, this));
}

inline BoolVarRef BooleanContainer::get_var_num(int i)
{
  D_ASSERT(!lock_m);
  D_ASSERT(i < (int)var_count_m);
  return BoolVarRef(BoolVarRef_internal(i, this));
}

inline BoolVarRef_internal::BoolVarRef_internal(int value, BooleanContainer* b_con) : 
  data_offset(value / (sizeof(data_type)*8)), var_num(value),  
  data_position(b_con->assign_offset, data_offset*sizeof(data_type)),
  value_position(b_con->values_mem, data_offset*sizeof(data_type))
{ shift_offset = one << (value % (sizeof(data_type)*8)); }

