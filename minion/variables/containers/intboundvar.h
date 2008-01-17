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

struct BoundVarRef_internal
{
  static const BOOL isBool = false;
  static const BoundType isBoundConst = Bound_Yes;
  BOOL isBound()
  { return true;}
  
  int var_num;
  BoundVarRef_internal() : var_num(-1)
  { }
  
  explicit BoundVarRef_internal(int i) : var_num(i)
  {}
};

struct GetBoundVarContainer;

#ifdef MORE_SEARCH_INFO
typedef InfoRefType<VarRefType<GetBoundVarContainer, BoundVarRef_internal>, VAR_INFO_BOUNDVAR> BoundVarRef;
#else
typedef VarRefType<GetBoundVarContainer, BoundVarRef_internal> BoundVarRef;
#endif

template<typename BoundType = int>
struct BoundVarContainer {
  BackTrackOffset bound_data;
  TriggerList trigger_list;
  vector<pair<BoundType, BoundType> > initial_bounds;
  unsigned var_count_m;
  BOOL lock_m;
  

  const BoundType& lower_bound(BoundVarRef_internal i) const
  { return static_cast<const BoundType*>(bound_data.get_ptr())[i.var_num*2]; }
  
  const BoundType& upper_bound(BoundVarRef_internal i) const
  { return static_cast<const BoundType*>(bound_data.get_ptr())[i.var_num*2 + 1]; }

  BoundType& lower_bound(BoundVarRef_internal i)
  { return static_cast<BoundType*>(bound_data.get_ptr())[i.var_num*2]; }
  
  BoundType& upper_bound(BoundVarRef_internal i)
  { return static_cast<BoundType*>(bound_data.get_ptr())[i.var_num*2 + 1]; }

  
  void lock()
  { 
    D_ASSERT(!lock_m);
    lock_m = true;
    bound_data.request_bytes(var_count_m*2*sizeof(BoundType));
    BoundType* bound_ptr = static_cast<BoundType*>(bound_data.get_ptr());
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
    
    trigger_list.lock(var_count_m, min_domain_val, max_domain_val);
    
  }
  
  BoundVarContainer() : lock_m(0), trigger_list(true)
  {}
  
  BOOL isAssigned(BoundVarRef_internal d) const
  { 
    D_ASSERT(lock_m);
    return lower_bound(d) == upper_bound(d); 
  }
  
  int getAssignedValue(BoundVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(isAssigned(d));
    return lower_bound(d);
  }
  
  BOOL inDomain(BoundVarRef_internal d, int i) const
  {
    D_ASSERT(lock_m);
    if (i < lower_bound(d) || i > upper_bound(d))
      return false;
    return true;
  }
  
  BOOL inDomain_noBoundCheck(BoundVarRef_internal d, int i) const
  {
    D_ASSERT(lock_m);
	D_ASSERT(i >= lower_bound(d));
	D_ASSERT(i <= upper_bound(d));
    return true;
  }
  
  int getMin(BoundVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(Controller::failed || inDomain(d,lower_bound(d)));
    return lower_bound(d);
  }
  
  int getMax(BoundVarRef_internal d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(Controller::failed || inDomain(d,upper_bound(d)));
    return upper_bound(d);
  }
 
  int getInitialMin(BoundVarRef_internal d) const
  { return initial_bounds[d.var_num].first; }
  
  int getInitialMax(BoundVarRef_internal d) const
  { return initial_bounds[d.var_num].second; }
   
  void removeFromDomain(BoundVarRef_internal, int )
  {
    D_FATAL_ERROR( "Cannot Remove Value from domain of a bound var");
    FAIL_EXIT();
  }
  
  void propogateAssign(BoundVarRef_internal d, int i)
  {
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

    if(min_val != i)
      trigger_list.push_lower(d.var_num, i - min_val);
    
    if(max_val != i)
      trigger_list.push_upper(d.var_num, max_val - i);
    
    upper_bound(d) = i;
    lower_bound(d) = i;
  }
  
  // TODO : Optimise
  void uncheckedAssign(BoundVarRef_internal d, int i)
  { 
    D_ASSERT(inDomain(d,i));
    propogateAssign(d,i); 
  }
  
  void setMax(BoundVarRef_internal d, int i)
  {
    int low_bound = lower_bound(d);
    int up_bound = upper_bound(d);
    
    if(i < low_bound)
    {
       Controller::fail();
       return;
    }
    
    
    if(i < up_bound)
    {
      trigger_list.push_upper(d.var_num, up_bound - i);
      trigger_list.push_domain(d.var_num);
      upper_bound(d) = i;
      if(low_bound == i)
	trigger_list.push_assign(d.var_num, i);
    }
  }
  
  void setMin(BoundVarRef_internal d, int i)
  {
    int low_bound = lower_bound(d);
    int up_bound = upper_bound(d);
    
    if(i > up_bound)
    {
      Controller::fail();
      return;
    }
    
    if(i > low_bound)
    {
      trigger_list.push_lower(d.var_num, i - low_bound);
      trigger_list.push_domain(d.var_num);
      lower_bound(d) = i;
      if(up_bound == i)
	    trigger_list.push_assign(d.var_num, i);
    }
  }
  
  BoundVarRef get_new_var();
  BoundVarRef get_new_var(int i, int j);
  BoundVarRef get_var_num(int i);

  void addTrigger(BoundVarRef_internal b, Trigger t, TrigType type)
  { 
	D_ASSERT(lock_m);  
	trigger_list.add_trigger(b.var_num, t, type); 
  }

#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(BoundVarRef_internal& b, DynamicTrigger* t, TrigType type, int pos = -999)
  {
	D_ASSERT(lock_m); 
	D_ASSERT(type != DomainRemoval);
	trigger_list.addDynamicTrigger(b.var_num, t, type, pos); 
  }
#endif

  operator std::string()
  {
    D_ASSERT(lock_m);
    stringstream s;
    int char_count = 0;
    for(unsigned int i=0;i<var_count_m;i++)
    {
      if(!isAssigned(BoundVarRef_internal(i)))
	s << "X";
      else
      {
	s << (getAssignedValue(BoundVarRef_internal(i))?1:0); 
      }
      char_count++;
      if(char_count%7==0) s << endl;
    }
    return s.str();
  }
  
};


VARDEF(BoundVarContainer<> boundvar_container);

struct GetBoundVarContainer
{
  static BoundVarContainer<>& con() 
  { return boundvar_container; }
  static string name()
  { return "Bound"; }
};

template<typename T>
inline BoundVarRef
BoundVarContainer<T>::get_new_var(int i, int j)
{
  D_ASSERT(!lock_m);
  D_ASSERT(i >= std::numeric_limits<T>::min());
  D_ASSERT(j <= std::numeric_limits<T>::max());
 // D_ASSERT(i >= var_min && j <= var_max);
  initial_bounds.push_back(make_pair(i,j));
  return BoundVarRef(BoundVarRef_internal(var_count_m++));
}

template<typename T>
inline BoundVarRef
BoundVarContainer<T>::get_var_num(int i)
{
  D_ASSERT(!lock_m);
  return BoundVarRef(BoundVarRef_internal(i));
}

