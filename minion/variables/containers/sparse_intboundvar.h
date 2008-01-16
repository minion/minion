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

struct SparseBoundVarRef_internal
{
  static const BOOL isBool = true;
  static const BoundType isBoundConst = Bound_Yes;
  
  BOOL isBound()
  { return true;}
  
  int var_num;
  SparseBoundVarRef_internal()
  { D_DATA(var_num = -1;) }
  
  explicit SparseBoundVarRef_internal(int i) : var_num(i)
  {}
};

struct GetSparseBoundVarContainer;

#ifdef MORE_SEARCH_INFO
typedef InfoRefType<VarRefType<GetSparseBoundVarContainer, SparseBoundVarRef_internal>, VAR_INFO_SPARSEBOUND> SparseBoundVarRef;
#else
typedef VarRefType<GetSparseBoundVarContainer, SparseBoundVarRef_internal> SparseBoundVarRef;
#endif

template<typename BoundType = int>
struct SparseBoundVarContainer {
  BackTrackOffset bound_data;
  TriggerList trigger_list;
  vector<vector<BoundType> > domains;
  vector<int> domain_reference;
  unsigned var_count_m;
  BOOL lock_m;
  
  vector<BoundType>& get_domain(SparseBoundVarRef_internal i)
  { return domains[domain_reference[i.var_num]]; }
 
  vector<BoundType>& get_domain_from_int(int i)
  { return domains[domain_reference[i]]; }
  
  const BoundType& lower_bound(SparseBoundVarRef_internal i) const
  { return static_cast<const BoundType*>(bound_data.get_ptr())[i.var_num*2]; }
  
  const BoundType& upper_bound(SparseBoundVarRef_internal i) const
  { return static_cast<const BoundType*>(bound_data.get_ptr())[i.var_num*2 + 1]; }
  
  BoundType& lower_bound(SparseBoundVarRef_internal i)
  { return static_cast<BoundType*>(bound_data.get_ptr())[i.var_num*2]; }
  
  BoundType& upper_bound(SparseBoundVarRef_internal i)
  { return static_cast<BoundType*>(bound_data.get_ptr())[i.var_num*2 + 1]; }

  /// find the small possible lower bound above @new_lower_bound.
  /// Does not actually change the lower bound.  
  int find_lower_bound(SparseBoundVarRef_internal d, int new_lower_bound)
  {
    vector<BoundType>& bounds = get_domain(d);
    typename vector<BoundType>::iterator it = std::lower_bound(bounds.begin(), bounds.end(), new_lower_bound);
    if(it == bounds.end())
    {
      Controller::fail();
      return *(it - 1);
    }
    
    return *it;
  }
  
  /// find the largest possible upper bound below @new_upper_bound.
  /// Does not actually change the upper bound.
  int find_upper_bound(SparseBoundVarRef_internal& d, int new_upper_bound)
  {
    vector<BoundType>& bounds = get_domain(d);

    typename vector<BoundType>::iterator it = std::lower_bound(bounds.begin(), bounds.end(), new_upper_bound);
    if(it == bounds.end())
      return *(it - 1);
    
    if(*it == new_upper_bound)
      return new_upper_bound;
    
    if(it == bounds.begin())
    {
      Controller::fail();
      return bounds.front();
    }
    
    return *(it - 1);
  }
  
  void lock()
  { 
    D_ASSERT(!lock_m);
    lock_m = true;
    
    int min_domain_val = 0;
    int max_domain_val = 0;
    if(var_count_m != 0)
    {
      min_domain_val = get_domain_from_int(0).front();
      max_domain_val = get_domain_from_int(0).back();
      for(unsigned int i = 0; i < var_count_m; ++i)
      {   
        min_domain_val = mymin(get_domain_from_int(i).front(), min_domain_val);
        max_domain_val = mymax(get_domain_from_int(i).back(), max_domain_val);
      }
    }
    
    bound_data.request_bytes(var_count_m*2*sizeof(BoundType));
    BoundType* bound_ptr = static_cast<BoundType*>(bound_data.get_ptr());
    for(unsigned int i = 0; i < var_count_m; ++i)
    {
      bound_ptr[2*i] = get_domain_from_int(i).front();
      bound_ptr[2*i+1] = get_domain_from_int(i).back();
    }
    
    trigger_list.lock(var_count_m, min_domain_val, max_domain_val);
  }
  
  SparseBoundVarContainer() : lock_m(0)
  {}
  
  BOOL isAssigned(SparseBoundVarRef_internal d) const
  { 
    D_ASSERT(lock_m);
    return lower_bound(d) == upper_bound(d); 
  }
  
  int getAssignedValue(SparseBoundVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(isAssigned(d));
    return lower_bound(d);
  }
  
  /// This function is provided just to make life simpler. It should never be called.
  BOOL inDomain(SparseBoundVarRef_internal, int) const
  { D_FATAL_ERROR( "sparse bound ints do not allow 'inDomain' checks"); }

  /// This function is provided just to make life simpler. It should never be called.
  BOOL inDomain_noBoundCheck(SparseBoundVarRef_internal, int) const
  { D_FATAL_ERROR("sparse bound ints do not allow 'inDomain_noBoundCheck' checks"); }
  
  int getMin(SparseBoundVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    return lower_bound(d);
  }
  
  int getMax(SparseBoundVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    return upper_bound(d);
  }

  int getInitialMin(SparseBoundVarRef_internal d)
  { return get_domain_from_int(d.var_num).front(); }
  
  int getInitialMax(SparseBoundVarRef_internal d)
  { return get_domain_from_int(d.var_num).back(); }
  
  /// This function is provided for convience. It should never be called.
  void removeFromDomain(SparseBoundVarRef_internal, int)
  {
    FAIL_EXIT();
  }
  
  
  void propogateAssign(SparseBoundVarRef_internal d, int i)
  {
    vector<BoundType>& bounds = get_domain(d);
    if(!binary_search(bounds.begin(), bounds.end(), i))
    {
      Controller::fail();
      return;
    }
    int min_val = getMin(d);
    int max_val = getMax(d);
    if(min_val > i || max_val < i)
    {
      Controller::fail();
      return;
    }
    
    if(min_val == max_val)
      return;
    
    trigger_list.push_domain(d.var_num);
    trigger_list.push_assign(d.var_num, i);
    
#ifdef FULL_DOMAIN_TRIGGERS
	// Can't attach triggers to bound vars!  
#endif
    
    if(min_val != i)
      trigger_list.push_lower(d.var_num, i - min_val);
    
    if(max_val != i)
      trigger_list.push_upper(d.var_num, max_val - i);    
    
    upper_bound(d) = i;
    lower_bound(d) = i;
  }
  
  // TODO : Optimise
  void uncheckedAssign(SparseBoundVarRef_internal d, int i)
  { propogateAssign(d,i); }
  
  void setMax(SparseBoundVarRef_internal d, int i)
  {
    // Note, this just finds a new upper bound, it doesn't set it.
    i = find_upper_bound(d, i);
    
    int low_bound = lower_bound(d);
    
    if(i < low_bound)
    {
      Controller::fail();
      return;
    }
    
    int up_bound = upper_bound(d);
    
    if(i < up_bound)
    {
      
      trigger_list.push_upper(d.var_num, up_bound - i);
      trigger_list.push_domain(d.var_num);
#ifdef FULL_DOMAIN_TRIGGERS
  // Can't attach triggers to bound vars!  
#endif

      upper_bound(d) = i;
      if(low_bound == i)
        trigger_list.push_assign(d.var_num, i);
    }
  }
  
  void setMin(SparseBoundVarRef_internal d, int i)
  {
    i = find_lower_bound(d,i);
    
    int up_bound = upper_bound(d);
    
    if(i > up_bound)
    {
      Controller::fail();
      return;
    }
    
    int low_bound = lower_bound(d);
    
    if(i > low_bound)
    {
      trigger_list.push_lower(d.var_num, i - low_bound);
      trigger_list.push_domain(d.var_num);
#ifdef FULL_DOMAIN_TRIGGERS
	  // Can't attach triggers to bound vars!  
#endif
      lower_bound(d) = i;
      if(up_bound == i)
       trigger_list.push_assign(d.var_num, i);
    }
  }
  
//  SparseBoundVarRef get_new_var();
  template<typename T>
  SparseBoundVarRef get_new_var(const vector<T>&);
  SparseBoundVarRef get_var_num(int i);

  void addTrigger(SparseBoundVarRef_internal b, Trigger t, TrigType type)
  { 
    D_ASSERT(lock_m); 
	trigger_list.add_trigger(b.var_num, t, type); 
  }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(SparseBoundVarRef_internal b, DynamicTrigger* t, TrigType type, int pos = -999)
  { D_ASSERT(lock_m); trigger_list.addDynamicTrigger(b.var_num, t, type, pos); }
#endif

  operator std::string()
  {
    D_ASSERT(lock_m);
    stringstream s;
    int char_count = 0;
    for(unsigned int i=0;i<var_count_m;i++)
    {
      if(!isAssigned(SparseBoundVarRef_internal(i)))
    s << "X";
      else
      {
    s << (getAssignedValue(SparseBoundVarRef_internal(i))?1:0); 
      }
      char_count++;
      if(char_count%7==0) s << endl;
    }
    return s.str();
  }
  
};


VARDEF(SparseBoundVarContainer<> sparse_boundvar_container);

struct GetSparseBoundVarContainer
{
  static SparseBoundVarContainer<>& con() 
  { return sparse_boundvar_container; }
  
  static string name()
  { return "SparseBound:"; }
};


template<typename T>
template<typename U>
inline SparseBoundVarRef
SparseBoundVarContainer<T>::get_new_var(const vector<U>& new_domain)
{
  D_ASSERT(!lock_m);
  // D_ASSERT(i >= var_min && j <= var_max);
  D_ASSERT(new_domain.front() >= std::numeric_limits<T>::min());
  D_ASSERT(new_domain.back() <= std::numeric_limits<T>::max());
  
  D_DATA(for(int loop=0;loop<(int)(new_domain.size()) - 1; ++loop) 
     { D_ASSERT(new_domain[loop] < new_domain[loop+1]); } );
  
  vector<T> t_dom(new_domain.size());
  for(unsigned int i = 0; i < new_domain.size(); ++i)
    t_dom[i] = new_domain[i];
          
  int i=0;
  while(i < (int)(domains.size()) && domains[i] != t_dom)
    i++;
  
  if(i == (int)domains.size())
    domains.push_back(t_dom);
    
  domain_reference.push_back(i);
  
  
  return SparseBoundVarRef(SparseBoundVarRef_internal(var_count_m++));
}

template<typename T>
inline SparseBoundVarRef
SparseBoundVarContainer<T>::get_var_num(int i)
{
  D_ASSERT(!lock_m);
  return SparseBoundVarRef(SparseBoundVarRef_internal(i));
}

