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

struct RangeVarRef_internal
{
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_No;
  BOOL isBound()
  { return false;}
  
  int var_num;
  RangeVarRef_internal() : var_num(-1)
  { }
  
  explicit RangeVarRef_internal(int i) : var_num(i)
  {}
};

struct GetRangeVarContainer;

#ifdef MORE_SEARCH_INFO
typedef InfoRefType<VarRefType<GetRangeVarContainer, RangeVarRef_internal>, VAR_INFO_RANGEVAR> LRangeVarRef;
#else
typedef VarRefType<GetRangeVarContainer, RangeVarRef_internal> LRangeVarRef;
#endif

template<int var_min, typename d_type>
struct RangeVarContainer {
  typedef unsigned char domain_type;
  static const int var_max = var_min + sizeof(d_type) * 8 - 1;
  static const d_type one = static_cast<d_type>(1);
  BackTrackOffset bound_data;
  BackTrackOffset val_data;
  TriggerList trigger_list;
  
  vector<pair<int,int> > initial_bounds;
  unsigned var_count_m;
  BOOL lock_m;
  
  domain_type& raw_lower_bound(RangeVarRef_internal i) const
  { return static_cast<domain_type*>(bound_data.get_ptr())[i.var_num*2]; }
  int lower_bound(RangeVarRef_internal i) const
  { return raw_lower_bound(i) + var_min; }
  domain_type& raw_upper_bound(RangeVarRef_internal i) const
  { return static_cast<domain_type*>(bound_data.get_ptr())[i.var_num*2 + 1]; }
  int upper_bound(RangeVarRef_internal i) const
  { return raw_upper_bound(i) + var_min; }
  
  d_type& __data(RangeVarRef_internal i) const
  { return static_cast<d_type*>(val_data.get_ptr())[i.var_num]; }
  
  bool in_bitarray(RangeVarRef_internal d, int val) const
  { return __data(d) & (one << val); }
  
  void remove_from_bitarray(RangeVarRef_internal d, int offset) const
  { __data(d) &= ~(one << offset); }
  
  /// Returns new upper bound.
  int find_new_raw_upper_bound(RangeVarRef_internal d)
  {
    int lower = raw_lower_bound(d);
	// d_type val = data(d);
    int old_loopvar = raw_upper_bound(d);
	int loopvar = old_loopvar;
    if(in_bitarray(d, loopvar) && (loopvar >= lower))
	{ return loopvar; }
    loopvar--;
    for(; loopvar >= lower; --loopvar)
    {
      if(in_bitarray(d, loopvar))
      { 
	    return loopvar;
      }
    }
    Controller::fail();
	return old_loopvar;
  }
  
  /// Returns true if lower bound is changed.
  int find_new_raw_lower_bound(RangeVarRef_internal d)
  {
    int upper = raw_upper_bound(d);
    //d_type val = data(d);
    int old_loopvar = raw_lower_bound(d);
	int loopvar = old_loopvar;
    if(in_bitarray(d, loopvar) && (loopvar <= upper))
	{ return loopvar; }
    loopvar++;
    for(; loopvar <= upper; ++loopvar)
    {
      if(in_bitarray(d, loopvar))
      { 
	    return loopvar;
      }
    }
    Controller::fail();
	return old_loopvar;
  }
    
  void lock()
  { 
    D_ASSERT(!lock_m);
    lock_m = true;

    bound_data.request_bytes(var_count_m*2);
    char* bound_ptr = static_cast<char*>(bound_data.get_ptr());
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
	
    val_data.request_bytes(var_count_m*sizeof(d_type));  
    d_type* val_ptr = static_cast<d_type*>(val_data.get_ptr());
    fill(val_ptr, val_ptr + var_count_m, ~static_cast<d_type>(0));
	
    trigger_list.lock(var_count_m, min_domain_val, max_domain_val);
  }
  
  RangeVarContainer() : lock_m(0), trigger_list(false)
  {}
  
  BOOL isAssigned(RangeVarRef_internal d) const
  { 
    D_ASSERT(lock_m);
    return lower_bound(d) == upper_bound(d); 
  }
  
  int getAssignedValue(RangeVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(isAssigned(d));
    return lower_bound(d);
  }
  
  BOOL inDomain(RangeVarRef_internal d, int i) const
  {
    D_ASSERT(lock_m);
    if (i < lower_bound(d) || i > upper_bound(d))
      return false;
    return in_bitarray(d,i - var_min);
  }
  
  BOOL inDomain_noBoundCheck(RangeVarRef_internal d, int i) const
  {
    D_ASSERT(lock_m);
	D_ASSERT(i >= lower_bound(d));
	D_ASSERT(i <= upper_bound(d));
    return in_bitarray(d,i - var_min);
  }

  
  
  int getMin(RangeVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(Controller::failed || inDomain(d,lower_bound(d)));
    return lower_bound(d);
  }
  
  int getMax(RangeVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(Controller::failed || inDomain(d,upper_bound(d)));
    return upper_bound(d);
  }

  int getInitialMin(RangeVarRef_internal d) const
  { return initial_bounds[d.var_num].first; }
  
  int getInitialMax(RangeVarRef_internal d) const
  { return initial_bounds[d.var_num].second; }
    
  void removeFromDomain(RangeVarRef_internal d, int i)
  {
    D_ASSERT(lock_m);
    if(!inDomain(d,i)) 
      return;
    int offset = i - var_min;
    trigger_list.push_domain(d.var_num);
#ifdef FULL_DOMAIN_TRIGGERS
	trigger_list.push_domain_removal(d.var_num, i);
#endif
	remove_from_bitarray(d, offset);
    domain_type up_bound = raw_upper_bound(d);
    if(offset == up_bound)
    {
      raw_upper_bound(d) = find_new_raw_upper_bound(d);
      trigger_list.push_upper(d.var_num, up_bound - raw_upper_bound(d));
    }
    
    domain_type low_bound = raw_lower_bound(d);
    if(offset == low_bound)
    {
      raw_lower_bound(d) = find_new_raw_lower_bound(d);
      trigger_list.push_lower(d.var_num, raw_lower_bound(d) - low_bound);
    }
    
    if(raw_upper_bound(d) == raw_lower_bound(d))
      trigger_list.push_assign(d.var_num, getAssignedValue(d));
    return;
  }
  
  void propogateAssign(RangeVarRef_internal d, int i)
  {
    int offset = i - var_min;
    if(!inDomain(d,i))
      {Controller::fail(); return;}
	
	int raw_lower = raw_lower_bound(d);
	int raw_upper = raw_upper_bound(d);
	
    if(offset == raw_lower && offset == raw_upper)
      return;
	if(offset < raw_lower || offset > raw_upper)
	{
	  Controller::fail();
	  return;
	}
    trigger_list.push_domain(d.var_num);
    trigger_list.push_assign(d.var_num, i);
#ifdef FULL_DOMAIN_TRIGGERS
	// TODO : Optimise this function to only check values in domain.
	int min_val = getMin(d);
	int max_val = getMax(d);
	for(int loop = min_val; loop <= max_val; ++loop)
	{
	  if(inDomain_noBoundCheck(d, loop) && i != loop)
	    trigger_list.push_domain_removal(d.var_num, loop);
	}
#endif
    if(offset != raw_lower)
    {
      trigger_list.push_lower(d.var_num, offset - raw_lower);
      raw_lower_bound(d) = offset;
    }
    
    if(offset != raw_upper)
    {
      trigger_list.push_upper(d.var_num, raw_upper - offset);
      raw_upper_bound(d) = offset;
    }
  }
  
  // TODO : Optimise
  void uncheckedAssign(RangeVarRef_internal d, int i)
  { 
    D_ASSERT(inDomain(d,i));
    propogateAssign(d,i); 
  }
  
  void setMax(RangeVarRef_internal d, int i)
  {
    int offset = i - var_min;
    int up_bound = raw_upper_bound(d);
    int low_bound = raw_lower_bound(d);
	
	if(offset < low_bound)
	{
	  Controller::fail();
	  return;
	}
	
    if(offset < up_bound)
    {
#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(int loop = i + 1; loop <= up_bound + var_min; ++loop)
	  {
	    if(inDomain_noBoundCheck(d, loop))
	      trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif	 
 
      raw_upper_bound(d) = offset;
      int raw_new_upper = find_new_raw_upper_bound(d);

#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(int loop = raw_new_upper + 1 + var_min; loop <i; ++loop)
	  {
		D_ASSERT(!inDomain_noBoundCheck(d, loop));
	    //if(inDomain_noBoundCheck(d, loop))
	    //  trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif

      raw_upper_bound(d) = raw_new_upper;
      trigger_list.push_domain(d.var_num);
      trigger_list.push_upper(d.var_num, up_bound - raw_upper_bound(d));
     
      if(raw_lower_bound(d) == raw_upper_bound(d))
	    trigger_list.push_assign(d.var_num, getAssignedValue(d));
    }
  }
  
  void setMin(RangeVarRef_internal d, int i)
  {
    int offset = i - var_min;
    int low_bound = raw_lower_bound(d);   
	int up_bound = raw_upper_bound(d);
	
	if(offset > up_bound)
	{
	  Controller::fail();
	  return;
	}
	
    if(offset > low_bound)
    {
#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(int loop = low_bound + var_min; loop < i; ++loop)
	  {
	    if(inDomain_noBoundCheck(d, loop))
	      trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif	 	  
	  
	  raw_lower_bound(d) = offset;
	  int raw_new_lower = find_new_raw_lower_bound(d);

#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(int loop = i; loop < raw_new_lower + var_min; ++loop)
	  {
		// XXX : Can this loop ever trigger??
		D_ASSERT(!inDomain_noBoundCheck(d, loop))
	    //if(inDomain_noBoundCheck(d, loop))
		//trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif
      raw_lower_bound(d) = raw_new_lower;

      trigger_list.push_domain(d.var_num);
      trigger_list.push_lower(d.var_num, raw_new_lower - low_bound);

      if(raw_lower_bound(d) == raw_upper_bound(d))
	    trigger_list.push_assign(d.var_num, getAssignedValue(d));
    }
  }
  
  LRangeVarRef get_var_num(int i);
  LRangeVarRef get_new_var(int i, int j);

  void addTrigger(RangeVarRef_internal b, Trigger t, TrigType type)
  { D_ASSERT(lock_m); trigger_list.add_trigger(b.var_num, t, type);  }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(RangeVarRef_internal& b, DynamicTrigger* t, TrigType type, int pos = -999)
  { 
    D_ASSERT(lock_m);
    D_ASSERT(b.var_num >= 0);
	D_ASSERT(b.var_num <= (int)var_count_m);
	D_ASSERT(type != DomainRemoval || (pos >= getInitialMin(b) && pos <= getInitialMax(b)));
    trigger_list.addDynamicTrigger(b.var_num, t, type, pos); 
  }
#endif
  
  BOOL valid_range(int lower, int upper)
  { return (lower >= var_min && upper <= var_max); }
};

typedef RangeVarContainer<0,unsigned long long> LRVCon;

VARDEF(LRVCon rangevar_container);

struct GetRangeVarContainer
{
  static LRVCon& con() { return rangevar_container; }
  static string name() { return "RangeVar:"; }
};

template<int var_min, typename T>
inline LRangeVarRef
RangeVarContainer<var_min,T>::get_new_var(int i, int j)
{
  D_ASSERT(!lock_m);
  D_ASSERT(i >= var_min && j <= var_max);
  D_INFO(2, DI_INTCONTAINER, "Adding int var, domain [" + to_string(i) + "," + to_string(j) + "]");
  initial_bounds.push_back(make_pair(i,j));
  return LRangeVarRef(RangeVarRef_internal(var_count_m++));
}

template<int var_min, typename T>
inline LRangeVarRef
RangeVarContainer<var_min,T>::get_var_num(int i)
{
  D_ASSERT(!lock_m);
  return LRangeVarRef(RangeVarRef_internal(i));
}

