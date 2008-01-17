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

/** @help variables;discrete Description
In discrete variables the domain ranges from the lower bound to the
upper bound specified, but the deletion of any domain element in this
range is permitted. This means that holes can be put in the domain of
these variables.
*/

/** @help variables;discrete Example
Declaration of a discrete variable called myvar containing values
1 through to 10 inclusive in input file:

DISCRETE myvar {1..10}

Use of this variable in a constraint:
eq(bool, 3)
*/

template<int var_min, typename d_type>
struct RangeVarContainer;

template<int var_min, typename d_type>
struct RangeVarRef_internal_template
{
  static const d_type one = static_cast<d_type>(1);
  typedef unsigned char domain_type;

  MoveablePointer bound_ptr;
  MoveablePointer data_ptr;
  int var_num;
  
#ifdef MANY_VAR_CONTAINERS
  RangeVarContainer<var_min, d_type>* rangeCon;
  RangeVarContainer<var_min, d_type>& getCon() const { return *rangeCon; }

  explicit RangeVarRef_internal_template(RangeVarContainer<var_min, d_type>* con, int i, domain_type* _bound_ptr,
                                         d_type* _data_ptr) : 
  rangeCon(con), var_num(i), bound_ptr(_bound_ptr), data_ptr(_data_ptr)
  {}
  
  void operator=(const RangeVarRef_internal_template& var) 
  {
    rangeCon = var.rangeCon;
    var_num = var.var_num;
    bound_ptr = var.bound_ptr;
    data_ptr = var.data_ptr;
  }
#else
  static RangeVarContainer<var_min, d_type>& getCon_Static();
  explicit RangeVarRef_internal_template(RangeVarContainer<var_min, d_type>* con, int i, domain_type* _bound_ptr,
                                         d_type* _data_ptr) : 
  var_num(i), bound_ptr(_bound_ptr), data_ptr(_data_ptr)
  {}
  
  void operator=(const RangeVarRef_internal_template& var) 
  {
    var_num = var.var_num;
    bound_ptr = var.bound_ptr;
    data_ptr = var.data_ptr;
  }
#endif
  
 // The following methods are lifted from the container below, to try to simplify copying methods from there.
  const domain_type& raw_lower_bound() const
  { return *static_cast<domain_type*>(bound_ptr.get_ptr()); }
  int lower_bound() const
  { return raw_lower_bound() + var_min; }

  const domain_type& raw_upper_bound() const
  { return *(static_cast<domain_type*>(bound_ptr.get_ptr()) + 1); }
  int upper_bound() const
  { return raw_upper_bound() + var_min; }
  
  const d_type& __data() const
  { return *static_cast<d_type*>(data_ptr.get_ptr()); }

   bool in_bitarray(DomainInt dom_val) const
  { 
	int val = checked_cast<int>(dom_val);
	D_ASSERT(val >= 0 && val < sizeof(d_type) * 8);
	return __data() & (one << val); 
  }


  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_No;
  static string name() { return "Range"; }
  BOOL isBound()
  { return false;}
  
  RangeVarRef_internal_template() : var_num(-1)
  { }

  BOOL isAssigned() const
  { return raw_lower_bound() == raw_upper_bound(); }
  
  DomainInt getAssignedValue() const
  {
    D_ASSERT(isAssigned());
    return lower_bound();
  }
  
  BOOL isAssignedValue(DomainInt i)
  { 
    return isAssigned() &&
    getAssignedValue() == i;
  }
  
   BOOL inDomain(DomainInt i) const
  {
    if (i < lower_bound() || i > upper_bound())
      return false;
    return in_bitarray(i - var_min);
  }
  
  BOOL inDomain_noBoundCheck(DomainInt i) const
  {
	D_ASSERT(i >= lower_bound());
	D_ASSERT(i <= upper_bound());
    return in_bitarray(i - var_min);
  }

  DomainInt getMin() const
  { return lower_bound(); }
  
  DomainInt getMax() const
  { return upper_bound(); }

  DomainInt getInitialMax() const
  { return GET_LOCAL_CON().getInitialMax(*this); }
  
  DomainInt getInitialMin() const
  { return GET_LOCAL_CON().getInitialMin(*this); }
  
  void setMax(DomainInt i)
  { GET_LOCAL_CON().setMax(*this,i); }
  
  void setMin(DomainInt i)
  { GET_LOCAL_CON().setMin(*this,i); }
  
  void uncheckedAssign(DomainInt b)
  { GET_LOCAL_CON().uncheckedAssign(*this, b); }
  
  void propagateAssign(DomainInt b)
  { GET_LOCAL_CON().propagateAssign(*this, b); }
  
  void removeFromDomain(DomainInt b)
  { GET_LOCAL_CON().removeFromDomain(*this, b); }
  
  void addTrigger(Trigger t, TrigType type)
  { GET_LOCAL_CON().addTrigger(*this, t, type); }

  friend std::ostream& operator<<(std::ostream& o, const RangeVarRef_internal_template& v)
  { return o << "RangeVar:" << v.var_num; }
    
  int getDomainChange(DomainDelta d)
  { return d.XXX_get_domain_diff(); }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  GET_LOCAL_CON().addDynamicTrigger(*this, t, type, pos); }
#endif

};

//typedef RangeVarRef_internal_template<0, BitContainerType> RangeVarRef_internal;

#ifdef MORE_SEARCH_INFO
typedef InfoRefType<RangeVarRef_internal_template<0, BitContainerType>, VAR_INFO_RANGEVAR> LRangeVarRef;
#else
typedef RangeVarRef_internal_template<0, BitContainerType> LRangeVarRef;
#endif

template<int var_min, typename d_type>
struct RangeVarContainer {
  typedef RangeVarRef_internal_template<0, BitContainerType> RangeVarRef_internal;

  StateObj* stateObj;
  
  RangeVarContainer(StateObj* _stateObj) : stateObj(_stateObj), lock_m(0), 
                                           trigger_list(stateObj, false), var_count_m(0)
  {}
  
  typedef unsigned char domain_type;
// In C++, defining constants in enums avoids some linkage issues.
  enum Constant { var_max = var_min + sizeof(d_type) * 8 - 1 };
  static const d_type one = static_cast<d_type>(1);
  MoveableArray<domain_type> bound_data;
  MoveableArray<d_type> val_data;
  TriggerList trigger_list;
  
  vector<pair<int,int> > initial_bounds;
  unsigned var_count_m;
  BOOL lock_m;
  
  const domain_type& raw_lower_bound(const RangeVarRef_internal& i) const
  { return bound_data[i.var_num * 2]; }
  domain_type& raw_lower_bound(const RangeVarRef_internal& i)
  { return bound_data[i.var_num * 2]; }
  int lower_bound(const RangeVarRef_internal& i) const
  { return raw_lower_bound(i) + var_min; }

  const domain_type& raw_upper_bound(const RangeVarRef_internal& i) const
  { return bound_data[i.var_num*2 + 1]; }
  domain_type& raw_upper_bound(const RangeVarRef_internal& i)
  { return bound_data[i.var_num*2 + 1]; }
  int upper_bound(const RangeVarRef_internal& i) const
  { return raw_upper_bound(i) + var_min; }
  
  const d_type& __data(const RangeVarRef_internal& i) const
  { return val_data[i.var_num]; }
  d_type& __data(const RangeVarRef_internal& i)
  { return val_data[i.var_num]; }

  bool in_bitarray(const RangeVarRef_internal& d, DomainInt dom_val) const
  { 
	int val = checked_cast<int>(dom_val);
	D_ASSERT(val >= 0 && val < sizeof(d_type) * 8);
	return __data(d) & (one << val); 
  }
  
  void remove_from_bitarray(const RangeVarRef_internal& d, DomainInt dom_offset)
  { 
	int offset = checked_cast<int>(dom_offset);
	D_ASSERT(offset >= 0 && offset < sizeof(d_type) * 8);
	__data(d) &= ~(one << offset); 
  }
  
  /// Returns new upper bound.
  int find_new_raw_upper_bound(const RangeVarRef_internal& d)
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
    getState(stateObj).setFailed(true);
	return old_loopvar;
  }
  
  /// Returns true if lower bound is changed.
  int find_new_raw_lower_bound(const RangeVarRef_internal& d)
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
    getState(stateObj).setFailed(true);
	return old_loopvar;
  }
     
  void lock()
  { 
    D_ASSERT(!lock_m);
    lock_m = true;
  }


  // These 'const' functions are all declared in RangeVarRef_internal, so long term
  // they could be removed from here. At the moment they are left here as the modifing
  // members use them, so it makes things easier.
  BOOL isAssigned(const RangeVarRef_internal& d) const
  { 
    D_ASSERT(lock_m);
    return lower_bound(d) == upper_bound(d); 
  }
  
  DomainInt getAssignedValue(const RangeVarRef_internal& d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(isAssigned(d));
    return lower_bound(d);
  }

  BOOL inDomain(const RangeVarRef_internal& d, DomainInt i) const
  {
    D_ASSERT(lock_m);
    if (i < lower_bound(d) || i > upper_bound(d))
      return false;
    return in_bitarray(d,i - var_min);
  }
  
  BOOL inDomain_noBoundCheck(const RangeVarRef_internal& d, DomainInt i) const
  {
    D_ASSERT(lock_m);
	D_ASSERT(i >= lower_bound(d));
	D_ASSERT(i <= upper_bound(d));
    return in_bitarray(d,i - var_min);
  }


  DomainInt getMin(const RangeVarRef_internal& d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(getState(stateObj).isFailed() || inDomain(d,lower_bound(d)));
    return lower_bound(d);
  }
  
  DomainInt getMax(const RangeVarRef_internal& d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(getState(stateObj).isFailed() || inDomain(d,upper_bound(d)));
    return upper_bound(d);
  }

  DomainInt getInitialMin(const RangeVarRef_internal& d) const
  { return initial_bounds[d.var_num].first; }
  
  DomainInt getInitialMax(const RangeVarRef_internal& d) const
  { return initial_bounds[d.var_num].second; }
    
  void removeFromDomain(const RangeVarRef_internal& d, DomainInt i)
  {
    D_ASSERT(lock_m);
    if(!inDomain(d,i)) 
      return;
    DomainInt offset = i - var_min;
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
  
  void propagateAssign(const RangeVarRef_internal& d, DomainInt i)
  {
    DomainInt offset = i - var_min;
    if(!inDomain(d,i))
      {getState(stateObj).setFailed(true); return;}
	
	int raw_lower = raw_lower_bound(d);
	int raw_upper = raw_upper_bound(d);
	
    if(offset == raw_lower && offset == raw_upper)
      return;
	if(offset < raw_lower || offset > raw_upper)
	{
	  getState(stateObj).setFailed(true);
	  return;
	}
    trigger_list.push_domain(d.var_num);
    trigger_list.push_assign(d.var_num, i);
#ifdef FULL_DOMAIN_TRIGGERS
	// TODO : Optimise this function to only check values in domain.
	DomainInt min_val = getMin(d);
	DomainInt max_val = getMax(d);
	for(DomainInt loop = min_val; loop <= max_val; ++loop)
	{
	  if(inDomain_noBoundCheck(d, loop) && i != loop)
	    trigger_list.push_domain_removal(d.var_num, loop);
	}
#endif
    if(offset != raw_lower)
    {
      trigger_list.push_lower(d.var_num, offset - raw_lower);
      raw_lower_bound(d) = checked_cast<unsigned char>(offset);
    }
    
    if(offset != raw_upper)
    {
      trigger_list.push_upper(d.var_num, raw_upper - offset);
      raw_upper_bound(d) = checked_cast<unsigned char>(offset);
    }
  }
  
  // TODO : Optimise
  void uncheckedAssign(const RangeVarRef_internal& d, DomainInt i)
  { 
    D_ASSERT(inDomain(d,i));
    propagateAssign(d,i); 
  }
  
  void setMax(const RangeVarRef_internal& d, DomainInt i)
  {
    DomainInt offset = i - var_min;
    DomainInt up_bound = raw_upper_bound(d);
    DomainInt low_bound = raw_lower_bound(d);
	
	if(offset < low_bound)
	{
	  getState(stateObj).setFailed(true);
	  return;
	}
	
    if(offset < up_bound)
    {
#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(DomainInt loop = i + 1; loop <= up_bound + var_min; ++loop)
	  {
	    if(inDomain_noBoundCheck(d, loop))
	      trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif	 
 
      raw_upper_bound(d) = checked_cast<unsigned char>(offset);
      int raw_new_upper = find_new_raw_upper_bound(d);

#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(int loop = raw_new_upper + 1 + var_min; loop <i; ++loop)
		D_ASSERT(!inDomain_noBoundCheck(d, loop));
#endif

      raw_upper_bound(d) = raw_new_upper;
      trigger_list.push_domain(d.var_num);
      trigger_list.push_upper(d.var_num, up_bound - raw_upper_bound(d));
     
      if(raw_lower_bound(d) == raw_upper_bound(d))
	    trigger_list.push_assign(d.var_num, getAssignedValue(d));
    }
  }
  
  void setMin(const RangeVarRef_internal& d, DomainInt i)
  {
    DomainInt offset = i - var_min;
    DomainInt low_bound = raw_lower_bound(d);   
	DomainInt up_bound = raw_upper_bound(d);
	
	if(offset > up_bound)
	{
	  getState(stateObj).setFailed(true);
	  return;
	}
	
    if(offset > low_bound)
    {
#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(DomainInt loop = low_bound + var_min; loop < i; ++loop)
	  {
	    if(inDomain_noBoundCheck(d, loop))
	      trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif	 	  
	  
	  raw_lower_bound(d) = checked_cast<unsigned char>(offset);
	  int raw_new_lower = find_new_raw_lower_bound(d);

#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
	  for(DomainInt loop = i; loop < raw_new_lower + var_min; ++loop)
	  {
		// XXX : Can this loop ever trigger??
		D_ASSERT(!inDomain_noBoundCheck(d, loop))
	    //if(inDomain_noBoundCheck(d, loop))
		//trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif
      raw_lower_bound(d) = checked_cast<unsigned char>(raw_new_lower);

      trigger_list.push_domain(d.var_num);
      trigger_list.push_lower(d.var_num, raw_new_lower - low_bound);

      if(raw_lower_bound(d) == raw_upper_bound(d))
	    trigger_list.push_assign(d.var_num, getAssignedValue(d));
    }
  }
  
  LRangeVarRef get_var_num(int i);
  LRangeVarRef get_new_var(int i, int j);

  void addTrigger(const RangeVarRef_internal& b, Trigger t, TrigType type)
  { D_ASSERT(lock_m); trigger_list.add_trigger(b.var_num, t, type);  }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(const RangeVarRef_internal& b, DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  { 
    D_ASSERT(lock_m);
    D_ASSERT(b.var_num >= 0);
	D_ASSERT(b.var_num <= (int)var_count_m);
	D_ASSERT(type != DomainRemoval || (pos >= getInitialMin(b) && pos <= getInitialMax(b)));
    trigger_list.addDynamicTrigger(b.var_num, t, type, pos); 
  }
#endif
  
  bool valid_range(DomainInt lower, DomainInt upper)
  { return (lower >= var_min && upper <= var_max); }

void addVariables(const vector<pair<int, Bounds> >& new_domains)
{
  D_ASSERT(!lock_m);

  int min_domain_val = 0;
  int max_domain_val = 0;

  if(!new_domains.empty())
  {
    min_domain_val = new_domains[0].second.lower_bound;
    max_domain_val = new_domains[0].second.upper_bound;
  }

  for(int i = 0; i < new_domains.size(); ++i)
  {
    D_ASSERT(new_domains[i].second.lower_bound >= var_min);
    D_ASSERT(new_domains[i].second.upper_bound <= var_max);

    for(int j = 0; j < new_domains[i].first; ++j)
    {
      var_count_m++;
      initial_bounds.push_back(make_pair(new_domains[i].second.lower_bound,
                                         new_domains[i].second.upper_bound));
      D_INFO(0,DI_INTCON,"Adding var of domain: (" + to_string(new_domains[i].second.lower_bound) + "," +
                                                     to_string(new_domains[i].second.upper_bound) + ")");
    }

    min_domain_val = mymin(new_domains[i].second.lower_bound, min_domain_val);
	max_domain_val = mymax(new_domains[i].second.upper_bound, max_domain_val);
  }


   // I am not storing 'btm' in a seperate object here for any kind of efficency reason.
   // In theory, I should be able to put the definitions of bound_data and val_data on one
   // line, but for some reason g++ produces the most bizarre compile-time error when I try.
   // One day, I might figure out why...
   BackTrackMemory& btm = getMemory(stateObj).backTrack();

   bound_data = btm.requestArray<domain_type>(var_count_m*2);
   val_data = btm.requestArray<d_type>(var_count_m);  
  
    for(unsigned int i = 0; i < var_count_m; ++i)
    {
      bound_data[2*i] = initial_bounds[i].first;
      bound_data[2*i+1] = initial_bounds[i].second;
	}
    	

    d_type* val_ptr = val_data.get_ptr();
    fill(val_ptr, val_ptr + var_count_m, ~static_cast<d_type>(0));
	
    trigger_list.lock(var_count_m, min_domain_val, max_domain_val);
  }
};

typedef RangeVarContainer<0, BitContainerType> LRVCon;

template<int var_min, typename T>
inline LRangeVarRef
RangeVarContainer<var_min,T>::get_var_num(int i)
{
  D_ASSERT(i < var_count_m);
  return LRangeVarRef(RangeVarRef_internal(this, i, &bound_data[i * 2], &val_data[i]));
}

