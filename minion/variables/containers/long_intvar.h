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
  BigRangeVarRef_internal() : var_num(-1)
  { }
  
  explicit BigRangeVarRef_internal(int i) : var_num(i)
  {}
};

struct GetBigRangeVarContainer;

#ifdef MORE_SEARCH_INFO
typedef InfoRefType<VarRefType<GetBigRangeVarContainer, BigRangeVarRef_internal>, VAR_INFO_BIGRANGEVAR> BigRangeVarRef;
#else
typedef VarRefType<GetBigRangeVarContainer, BigRangeVarRef_internal> BigRangeVarRef;
#endif

template<typename d_type>
struct BigRangeVarContainer {
  typedef DomainInt domain_bound_type;
  static const int var_step = sizeof(d_type) * 8;
  static const d_type one = static_cast<d_type>(1);
  BackTrackOffset bound_data;
  // MemOffset bms_pointers;
  MonotonicSet bms_array;
  TriggerList trigger_list;

  
  /// Initial bounds of each variable
  vector<pair<int,int> > initial_bounds;
  /// Position in the variable data (in counts of d_type) of where each variable starts
  vector<int> var_offset;
 
  unsigned var_count_m;
  BOOL lock_m;
  
    BigRangeVarContainer() : var_count_m(0), lock_m(0), trigger_list(false)
  { 
    // Store where the first variable will go.
    var_offset.push_back(0);
  }
  
  

  domain_bound_type& lower_bound(BigRangeVarRef_internal i) const
  { return static_cast<domain_bound_type*>(bound_data.get_ptr())[i.var_num*2]; }
  
  domain_bound_type& upper_bound(BigRangeVarRef_internal i) const
  { return static_cast<domain_bound_type*>(bound_data.get_ptr())[i.var_num*2 + 1]; }
    
  
  
  
  
  /// Find new "true" upper bound.
  /// This should be used by first setting the value of upper_bound(d), then calling
  /// this function to move this value past any removed values.
  DomainInt find_new_upper_bound(BigRangeVarRef_internal d)
  {
    DomainInt lower = lower_bound(d); 
    DomainInt old_up_bound = upper_bound(d);
    DomainInt loopvar = old_up_bound; 
    DomainInt low_bound = initial_bounds[d.var_num].first; 
    if(loopvar < lower)
	{
	  Controller::fail();
	  /// Here just remove the value which should lead to the least work.
	  return upper_bound(d);
	}
    /// Note: before calling isMember, remove the lower initial bound from the offset.
    //if(bms_pointer(d)->isMember(loopvar - low_bound) && (loopvar >= lower))
    if(bms_array.isMember(var_offset[d.var_num] + loopvar - low_bound) && (loopvar >= lower))
      return upper_bound(d);
    --loopvar;
    for(; loopvar >= lower; --loopvar)
    {
      //if(bms_pointer(d)->isMember(loopvar - low_bound)) 
      if(bms_array.isMember(var_offset[d.var_num] + loopvar - low_bound)) 
        return loopvar;
    }
    Controller::fail();
    return old_up_bound;
  }
  
  /// Find new "true" lower bound.
  /// This should be used by first setting the value of lower_bound(d), then calling
  /// this function to move this value past any removed values.
  DomainInt find_new_lower_bound(BigRangeVarRef_internal d)
  {
    DomainInt upper = upper_bound(d); 
    DomainInt old_low_bound = lower_bound(d);
    DomainInt loopvar = old_low_bound; 
    DomainInt low_bound = initial_bounds[d.var_num].first; 
    if(loopvar > upper)
	{
	  Controller::fail();
	  /// Here just remove the value which should lead to the least work.
	  return lower_bound(d);
	}
  /// Note: before calling isMember, remove the lower initial bound from the offset.
    //if(bms_pointer(d)->isMember(loopvar - low_bound) && (loopvar <= upper))
    if(bms_array.isMember(var_offset[d.var_num] + loopvar - low_bound) && (loopvar <= upper))
      return lower_bound(d);
    ++loopvar;
    for(; loopvar <= upper; ++loopvar)
    {
      if(bms_array.isMember(var_offset[d.var_num] + loopvar - low_bound)) 
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
    bms_array.initialise(var_offset.back(), var_offset.back());
    domain_bound_type* bound_ptr = static_cast<domain_bound_type*>(bound_data.get_ptr());
    for(unsigned int i = 0; i < var_count_m; ++i)
    {
      bound_ptr[2*i] = initial_bounds[i].first;
      bound_ptr[2*i+1] = initial_bounds[i].second;
#ifdef DEBUG 
  cout << "About to create new BMS " << endl;
#endif
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
	
    trigger_list.lock(var_count_m, min_domain_val, max_domain_val);
  }
  
  BOOL isAssigned(BigRangeVarRef_internal d) const
  { 
    D_ASSERT(lock_m);
    return lower_bound(d) == upper_bound(d); 
  }
  
  DomainInt getAssignedValue(BigRangeVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(isAssigned(d));
    return getMin(d);
  }
  
  BOOL inDomain(BigRangeVarRef_internal d, DomainInt i) const
  {
    D_ASSERT(lock_m);
    if (i < lower_bound(d) || i > upper_bound(d))
      return false;
    return bms_array.isMember(var_offset[d.var_num] + i - initial_bounds[d.var_num].first);
  }
  
  // Warning: If this is ever changed, be sure to check through the code for other places
  // where bms_array is used directly.
  BOOL inDomain_noBoundCheck(BigRangeVarRef_internal d, DomainInt i) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(i >= lower_bound(d));
    D_ASSERT(i <= upper_bound(d));
    return bms_array.isMember(var_offset[d.var_num] + i - initial_bounds[d.var_num].first);
  }
  
  DomainInt getMin(BigRangeVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    return lower_bound(d);
  }
  
  DomainInt getMax(BigRangeVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    return upper_bound(d);
  }
  
  DomainInt getInitialMin(BigRangeVarRef_internal d) const
  { return initial_bounds[d.var_num].first; }
  
  DomainInt getInitialMax(BigRangeVarRef_internal d) const
  { return initial_bounds[d.var_num].second; }
   
  void removeFromDomain(BigRangeVarRef_internal d, DomainInt i)
  {
#ifdef DEBUG
    cout << "Calling removeFromDomain: " << d.var_num << " " << i << " [" 
         << lower_bound(d) << ":" << upper_bound(d) << "] original ["
         << getInitialMin(d) << ":" << getInitialMax(d) << "]"
         << endl;
    //bms_pointer(d)->print_state();
    bms_array.print_state();
#endif
    D_ASSERT(lock_m);
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    if(!inDomain(d,i)) 
    {
#ifdef DEBUG
      cout << "Exiting removeFromDomain: " << d.var_num << " nothing to do" << endl;
#endif
      return;
    }
    DomainInt offset = i;
#ifdef FULL_DOMAIN_TRIGGERS
	trigger_list.push_domain_removal(d.var_num, i);
#endif
    trigger_list.push_domain(d.var_num);
    //bms_pointer(d)->remove(offset - initial_bounds[d.var_num].first);
    bms_array.remove(var_offset[d.var_num] + offset - initial_bounds[d.var_num].first);
    D_ASSERT( ! bms_array.isMember(var_offset[d.var_num] + offset - initial_bounds[d.var_num].first));
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

#ifdef DEBUG
    cout << "Exiting removeFromDomain: " << d.var_num << " " << i << " [" 
         << lower_bound(d) << ":" << upper_bound(d) << "] original ["
         << getInitialMin(d) << ":" << getInitialMax(d) << "]"
         << endl;
    bms_array.print_state();
#endif
    return;
  }
  
  void propogateAssign(BigRangeVarRef_internal d, DomainInt offset)
  {
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    if(!inDomain(d,offset))
	  {Controller::fail(); return;}
	DomainInt lower = lower_bound(d);
	DomainInt upper = upper_bound(d);
    if(offset == upper && offset == lower)
      return;
	
	if(offset > upper || offset < lower)
	{
	  Controller::fail();
	  return;
	}
    commonAssign(d, offset, lower, upper);
  }

  void uncheckedAssign(BigRangeVarRef_internal d, DomainInt i)
  { 
    D_ASSERT(inDomain(d,i));
    D_ASSERT(!isAssigned(d));
    commonAssign(d,i, lower_bound(d), upper_bound(d)); 
  }
    
private:
  // This function just unifies part of propogateAssign and uncheckedAssign
  void commonAssign(BigRangeVarRef_internal d, DomainInt offset, DomainInt lower, DomainInt upper)
  {
#ifdef FULL_DOMAIN_TRIGGERS
    // TODO : Optimise this function to only check values in domain.
    int domainOffset = var_offset[d.var_num] - initial_bounds[d.var_num].first;
    for(DomainInt loop = lower; loop <= upper; ++loop)
    {
      // def of inDomain: bms_array.isMember(var_offset[d.var_num] + i - initial_bounds[d.var_num].first);
      if(bms_array.isMember(loop + domainOffset) && loop != offset)
        trigger_list.push_domain_removal(d.var_num, loop);
    }
#endif
    trigger_list.push_domain(d.var_num);
    trigger_list.push_assign(d.var_num, offset);
    
    DomainInt low_bound = lower_bound(d);
    if(offset != low_bound)
    {
      trigger_list.push_lower(d.var_num, offset - low_bound);
      lower_bound(d) = offset;
    }
    
    DomainInt up_bound = upper_bound(d);
    if(offset != up_bound)
    {
      trigger_list.push_upper(d.var_num, up_bound - offset);
      upper_bound(d) = offset;
    }
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
  }    
public:

  
  void setMax(BigRangeVarRef_internal d, DomainInt offset)
  {
#ifdef DEBUG
    cout << "Calling setMax: " << d.var_num << " " << offset << " [" 
         << lower_bound(d) << ":" << upper_bound(d) << "] original ["
         << getInitialMin(d) << ":" << getInitialMax(d) << "]"
         << endl;
    bms_array.print_state();
#endif

    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
    DomainInt up_bound = upper_bound(d);
    DomainInt low_bound = lower_bound(d);
	
	if(offset < low_bound)
	{
	  Controller::fail();
	  return;
    }
	
    if(offset < up_bound)
    {
#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
      int domainOffset = var_offset[d.var_num] - initial_bounds[d.var_num].first;
	  for(DomainInt loop = offset + 1; loop <= up_bound; ++loop)
	  {
        // Def of inDomain: bms_array.isMember(var_offset[d.var_num] + i - initial_bounds[d.var_num].first);
	    if(bms_array.isMember(domainOffset + loop))
	      trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif	 
      upper_bound(d) = offset;      
	  DomainInt new_upper = find_new_upper_bound(d);
	  upper_bound(d) = new_upper;
      
      trigger_list.push_domain(d.var_num);
      trigger_list.push_upper(d.var_num, up_bound - upper_bound(d));
	  
      if(lower_bound(d) == upper_bound(d)) 
        trigger_list.push_assign(d.var_num, getAssignedValue(d));
    }
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
#ifdef DEBUG
    cout << "Exiting setMax: " << d.var_num << " " << upper_bound(d) << " [" 
         << lower_bound(d) << ":" << upper_bound(d) << "] original ["
         << getInitialMin(d) << ":" << getInitialMax(d) << "]"
         << endl;
    bms_array.print_state();
#endif
  }
  
  void setMin(BigRangeVarRef_internal d, DomainInt offset)
  {
#ifdef DEBUG
    cout << "Calling setMin: " << d.var_num << " " << offset << " [" 
         << lower_bound(d) << ":" << upper_bound(d) << "] original ["
         << getInitialMin(d) << ":" << getInitialMax(d) << "]"
         << endl;
    bms_array.print_state();
#endif
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );

	DomainInt up_bound = upper_bound(d);
    DomainInt low_bound = lower_bound(d);
    
	if(offset > up_bound)
	{
	  Controller::fail();
	  return;
	}
	
    if(offset > low_bound)
    {
#ifdef FULL_DOMAIN_TRIGGERS
	  // TODO : Optimise this function to only check values in domain.
      int domainOffset = var_offset[d.var_num] - initial_bounds[d.var_num].first;
	  for(DomainInt loop = low_bound; loop < offset; ++loop)
	  {
        // def of inDomain: bms_array.isMember(var_offset[d.var_num] + i - initial_bounds[d.var_num].first);
	    if(bms_array.isMember(loop + domainOffset))
	      trigger_list.push_domain_removal(d.var_num, loop);
	  }
#endif
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );

    lower_bound(d) = offset;
    DomainInt new_lower = find_new_lower_bound(d);    
    lower_bound(d) = new_lower; 
    
    trigger_list.push_domain(d.var_num); 
    trigger_list.push_lower(d.var_num, lower_bound(d) - low_bound);
    if(lower_bound(d) == upper_bound(d)) 
      trigger_list.push_assign(d.var_num, getAssignedValue(d)); 
    }
    D_ASSERT(Controller::failed || ( inDomain(d, lower_bound(d)) && inDomain(d, upper_bound(d)) ) );
#ifdef DEBUG
    cout << "Exiting setMin: " << d.var_num << " " << lower_bound(d) << " [" 
         << lower_bound(d) << ":" << upper_bound(d) << "] original ["
         << getInitialMin(d) << ":" << getInitialMax(d) << "]"
         << endl;
    bms_array.print_state();
#endif
  }
  
  BigRangeVarRef get_var_num(int i);
  BigRangeVarRef get_new_var(int i, int j);

  void addTrigger(BigRangeVarRef_internal b, Trigger t, TrigType type)
  { D_ASSERT(lock_m); trigger_list.add_trigger(b.var_num, t, type);  }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(BigRangeVarRef_internal b, DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  {  
    D_ASSERT(lock_m);
	D_ASSERT(b.var_num >= 0);
	D_ASSERT(b.var_num <= (int)var_count_m);
	D_ASSERT(type != DomainRemoval || (pos >= getInitialMin(b) && pos <= getInitialMax(b)));
    trigger_list.addDynamicTrigger(b.var_num, t, type, pos); 
  }
#endif

  ~BigRangeVarContainer() { 
    for(unsigned i=0; i < var_count_m ; i++) {
      // delete(bms_pointer(i));                // should delete space really!
    } ;
  }
};

typedef BigRangeVarContainer<BitContainerType> BigRangeCon;

VARDEF(BigRangeCon big_rangevar_container);

struct GetBigRangeVarContainer
{
  static BigRangeCon& con() { return big_rangevar_container; }
  static string name() { return "BigRangeVar"; }
};

template<typename T>
inline BigRangeVarRef
BigRangeVarContainer<T>::get_new_var(int i, int j)
{
  D_ASSERT(!lock_m);
 // D_ASSERT(i >= var_min && j <= var_max);
  initial_bounds.push_back(make_pair(i,j));
  int domain_size;
  domain_size = j - i + 1;
  var_offset.push_back( var_offset.back() + domain_size);
  return BigRangeVarRef(BigRangeVarRef_internal(var_count_m++));
}

template<typename T>
inline BigRangeVarRef
BigRangeVarContainer<T>::get_var_num(int i)
{
  D_ASSERT(!lock_m);
  return BigRangeVarRef(BigRangeVarRef_internal(i));
}
