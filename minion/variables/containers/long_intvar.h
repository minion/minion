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


struct BigRangeVarRef_internal
{
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_No;
  BOOL isBound()
  { return false;}
  
  int var_num;
  BigRangeVarRef_internal()
  { D_DATA(var_num = -1;) }
  
  explicit BigRangeVarRef_internal(int i) : var_num(i)
  {}
};

struct GetBigRangeVarContainer;
typedef VarRefType<GetBigRangeVarContainer, BigRangeVarRef_internal> BigRangeVarRef;

template<typename d_type>
struct BigRangeVarContainer {
  typedef short domain_bound_type;
  static const int var_step = sizeof(d_type) * 8;
  static const d_type one = static_cast<d_type>(1);
  BackTrackOffset bound_data;
  BackTrackOffset val_data;
  TriggerList trigger_list;
  
  /// Initial bounds of each variable
  vector<pair<int,int> > initial_bounds;
  /// Position in the variable data (in counts of d_type) of where each variable starts
  vector<int> var_offset;
  unsigned var_count_m;
  BOOL lock_m;
  
    BigRangeVarContainer() : var_count_m(0), lock_m(0)
  { 
    // Store where the first variable will go.
    var_offset.push_back(0);
  }
  
  
  domain_bound_type& lower_bound(BigRangeVarRef_internal i) const
  { return static_cast<domain_bound_type*>(bound_data.get_ptr())[i.var_num*2]; }
  
  domain_bound_type& upper_bound(BigRangeVarRef_internal i) const
  { return static_cast<domain_bound_type*>(bound_data.get_ptr())[i.var_num*2 + 1]; }
    
  d_type& __data(int var_num, int data_pos) const
  { return static_cast<d_type*>(val_data.get_ptr())[var_offset[var_num] + data_pos]; }
  
  
  /// Note: before calling this function, remove the lower initial bound from the offset.
  bool in_bitarray(BigRangeVarRef_internal d, int offset) const
  { 
    D_ASSERT(offset >= 0 && offset <= (initial_bounds[d.var_num].second - 
									   initial_bounds[d.var_num].first));
    unsigned var_num = d.var_num;
    unsigned shift = offset % var_step;
    unsigned data_pos = offset / var_step;
    return __data(var_num, data_pos) & (one << shift); 
  }
  
  /// Note: before calling this function, remove the initial lower bound from the offset.
  void remove_from_bitarray(BigRangeVarRef_internal d, int offset)
  { 
    D_ASSERT(offset >= 0 && offset <= (initial_bounds[d.var_num].second - 
									   initial_bounds[d.var_num].first));
    unsigned var_num = d.var_num;
	unsigned shift = offset % var_step;
	unsigned data_pos = offset / var_step;
	__data(var_num, data_pos) &= ~(one << shift);
  }
  
  /// Find new "true" upper bound.
  /// This should be used by first setting the value of upper_bound(d), then calling
  /// this function to move this value past any removed values.
  int find_new_upper_bound(BigRangeVarRef_internal d)
  {
    int lower = lower_bound(d);
	int old_up_bound = upper_bound(d);
    int loopvar = old_up_bound;
	int low_bound = initial_bounds[d.var_num].first;
	if(loopvar < lower)
	{
	  Controller::fail();
	  /// Here just remove the value which should lead to the least work.
	  return upper_bound(d);
	}
    if(in_bitarray(d, loopvar - low_bound) && (loopvar >= lower))
      return upper_bound(d);
    loopvar--;
    for(; loopvar >= lower; --loopvar)
    {
      if(in_bitarray(d, loopvar - low_bound))
		return loopvar;
    }
    Controller::fail();
    return old_up_bound;
  }
  
  /// Find new "true" lower bound.
  /// This should be used by first setting the value of lower_bound(d), then calling
  /// this function to move this value past any removed values.
  int find_new_lower_bound(BigRangeVarRef_internal d)
  {
    int upper = upper_bound(d);
	int old_low_bound = lower_bound(d);
    int loopvar = old_low_bound;
	int low_bound = initial_bounds[d.var_num].first;
	if(loopvar > upper)
	{
	  Controller::fail();
	  return loopvar;
	}
    if(in_bitarray(d, loopvar - low_bound) && (loopvar <= upper))
      return loopvar;
    loopvar++;
    for(; loopvar <= upper; ++loopvar)
    {
      if(in_bitarray(d, loopvar - low_bound))
		return loopvar;
    }
    Controller::fail();
    return old_low_bound;
  }
  
  
  void lock()
  { 
    D_ASSERT(!lock_m);
    lock_m = true;
    bound_data.request_bytes(var_count_m * 2 * sizeof(domain_bound_type));
    domain_bound_type* bound_ptr = static_cast<domain_bound_type*>(bound_data.get_ptr());
    for(unsigned int i = 0; i < var_count_m; ++i)
    {
      bound_ptr[2*i] = initial_bounds[i].first;
      bound_ptr[2*i+1] = initial_bounds[i].second;
    }
    
	int min_domain_val = 0;
	int max_domain_val = 0;
	if(!initial_bounds.empty())
	{
	  min_domain_val = initial_bounds[0].first;
	  max_domain_val = initial_bounds[0].second;
	  for(unsigned int i = 0; i < var_count_m; ++i)
      {
        bound_ptr[2*i] = initial_bounds[i].first;
        bound_ptr[2*i+1] = initial_bounds[i].second;
	  
	    min_domain_val = mymin(initial_bounds[i].first, min_domain_val);
	    max_domain_val = mymax(initial_bounds[i].second, max_domain_val);
      }
    }
	
    val_data.request_bytes(var_offset.back() * sizeof(d_type));  
    d_type* val_ptr = static_cast<d_type*>(val_data.get_ptr());
    fill(val_ptr, val_ptr + var_offset.back(), ~static_cast<d_type>(0));
    trigger_list.lock(var_count_m, min_domain_val, max_domain_val);
  }
  
  BOOL isAssigned(BigRangeVarRef_internal d) const
  { 
    D_ASSERT(lock_m);
    return lower_bound(d) == upper_bound(d); 
  }
  
  int getAssignedValue(BigRangeVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(isAssigned(d));
    return getMin(d);
  }
  
  BOOL inDomain(BigRangeVarRef_internal d, int i) const
  {
    D_ASSERT(lock_m);
    if (i < lower_bound(d) || i > upper_bound(d))
      return false;
    return in_bitarray(d,i - initial_bounds[d.var_num].first);
  }
  
  BOOL inDomain_noBoundCheck(BigRangeVarRef_internal d, int i) const
  {
    D_ASSERT(lock_m);
	D_ASSERT(i >= lower_bound(d));
	D_ASSERT(i <= upper_bound(d));
    return in_bitarray(d,i - initial_bounds[d.var_num].first);
  }
  
  int getMin(BigRangeVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    return lower_bound(d);
  }
  
  int getMax(BigRangeVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    return upper_bound(d);
  }
  
  int getInitialMin(BigRangeVarRef_internal d) const
  { return initial_bounds[d.var_num].first; }
  
  int getInitialMax(BigRangeVarRef_internal d) const
  { return initial_bounds[d.var_num].second; }
   
  void removeFromDomain(BigRangeVarRef_internal d, int i)
  {
    D_ASSERT(lock_m);
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    if(!inDomain(d,i)) 
      return;
    int offset = i;
#ifdef FULL_DOMAIN_TRIGGERS
	trigger_list.push_domain_removal(d.var_num, i);
#endif
    trigger_list.push_domain(d.var_num);
    remove_from_bitarray(d, offset - initial_bounds[d.var_num].first);
    domain_bound_type up_bound = upper_bound(d);
    if(offset == up_bound)
    {
      upper_bound(d) = find_new_upper_bound(d);
      trigger_list.push_upper(d.var_num, up_bound - upper_bound(d));
    }
    
    domain_bound_type low_bound = lower_bound(d);
    if(offset == low_bound)
    {
      lower_bound(d) = find_new_lower_bound(d);
      trigger_list.push_lower(d.var_num, lower_bound(d) - low_bound);
    }
    
    if(upper_bound(d) == lower_bound(d))
      trigger_list.push_assign(d.var_num, getAssignedValue(d));
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    return;
  }
  
  void propogateAssign(BigRangeVarRef_internal d, int offset)
  {
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    if(!inDomain(d,offset))
	  {Controller::fail(); return;}
	int lower = lower_bound(d);
	int upper = upper_bound(d);
    if(offset == upper && offset == lower)
      return;
#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(int loop = lower; loop <= upper; ++loop)
	  {
	    if(inDomain(d, loop) && loop != offset)
	      trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif
    trigger_list.push_domain(d.var_num);
    trigger_list.push_assign(d.var_num, offset);
    
    int low_bound = lower_bound(d);
    if(offset != low_bound)
    {
      trigger_list.push_lower(d.var_num, offset - low_bound);
      lower_bound(d) = offset;
    }
    
    int up_bound = upper_bound(d);
    if(offset != up_bound)
    {
      trigger_list.push_upper(d.var_num, up_bound - offset);
      upper_bound(d) = offset;
    }
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
  }
  
  // TODO : Optimise
  void uncheckedAssign(BigRangeVarRef_internal d, int i)
  { 
    D_ASSERT(inDomain(d,i));
    propogateAssign(d,i); 
  }
  
  void setMax(BigRangeVarRef_internal d, int offset)
  {
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    int up_bound = upper_bound(d);
    
    if(offset < up_bound)
    {
#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(int loop = offset + 1; loop <= up_bound; ++loop)
	  {
	    if(inDomain(d, loop))
	      trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif	 

      upper_bound(d) = offset;      
	  int new_upper = find_new_upper_bound(d);

#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(int loop = new_upper + 1; loop <= offset; ++loop)
	  {
	    if(inDomain(d, loop))
	      trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif

	  upper_bound(d) = new_upper;
      trigger_list.push_domain(d.var_num);
      trigger_list.push_upper(d.var_num, up_bound - upper_bound(d));
	  
      if(lower_bound(d) == upper_bound(d))
		trigger_list.push_assign(d.var_num, getAssignedValue(d));
    }
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
  }
  
  void setMin(BigRangeVarRef_internal d, int offset)
  {
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    int low_bound = lower_bound(d);    
	
    if(offset > low_bound)
    {
#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(int loop = low_bound; loop < offset; ++loop)
	  {
	    if(inDomain(d, loop))
	      trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif

      lower_bound(d) = offset;
      int new_lower = find_new_lower_bound(d);
	  
#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(int loop = offset; loop < new_lower; ++loop)
	  {
	    if(inDomain(d, loop))
	      trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif
	  
	  lower_bound(d) = new_lower;
      trigger_list.push_domain(d.var_num);
      trigger_list.push_lower(d.var_num, lower_bound(d) - low_bound);
	  
      if(lower_bound(d) == upper_bound(d))
		trigger_list.push_assign(d.var_num, getAssignedValue(d));
	  
    }
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
  }
  
  BigRangeVarRef get_var_num(int i);
  BigRangeVarRef get_new_var(int i, int j);

  void addTrigger(BigRangeVarRef_internal b, Trigger t, TrigType type)
  { D_ASSERT(lock_m); trigger_list.add_trigger(b.var_num, t, type);  }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(BigRangeVarRef_internal b, DynamicTrigger* t, TrigType type, int pos = -999)
  {  
    D_ASSERT(lock_m);
	D_ASSERT(b.var_num >= 0);
	D_ASSERT(b.var_num <= (int)var_count_m);
	D_ASSERT(type != DomainRemoval || (pos >= getInitialMin(b) && pos <= getInitialMax(b)));
    trigger_list.addDynamicTrigger(b.var_num, t, type, pos); 
  }
#endif
};

typedef BigRangeVarContainer<unsigned long long> BigRangeCon;

VARDEF(BigRangeCon big_rangevar_container);

struct GetBigRangeVarContainer
{
  static BigRangeCon& con() { return big_rangevar_container; }
};

template<typename T>
inline BigRangeVarRef
BigRangeVarContainer<T>::get_new_var(int i, int j)
{
  D_ASSERT(!lock_m);
 // D_ASSERT(i >= var_min && j <= var_max);
  initial_bounds.push_back(make_pair(i,j));
  int domain_size = j - i + 1;
  if( (domain_size) % (8 * sizeof(T)) == 0)
    var_offset.push_back( var_offset.back() + domain_size / (8 * sizeof(T)) );
  else
    var_offset.push_back( var_offset.back() + 1 + domain_size / (8 * sizeof(T)) );
  return BigRangeVarRef(BigRangeVarRef_internal(var_count_m++));
}

template<typename T>
inline BigRangeVarRef
BigRangeVarContainer<T>::get_var_num(int i)
{
  D_ASSERT(!lock_m);
  return BigRangeVarRef(BigRangeVarRef_internal(i));
}

