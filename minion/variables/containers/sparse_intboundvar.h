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

/** @help variables;sparsebounds Description
In sparse bounds variables the domain is composed of discrete values
(e.g. {1, 5, 36, 92}), but only the upper and lower bounds of the
domain may be updated during search. Although the domain of these
variables is not a continuous range, any holes in the domains must be
there at time of specification, as they can not be added during the
solving process.
*/

/** @help variables;sparsebounds Notes 
Declaration of a sparse bounds variable called myvar containing values
{1,3,4,6,7,9,11} in input file:

SPARSEBOUND myvar {1,3,4,6,7,9,11}

Use of this variable in a constraint:
eq(myvar, 3) #myvar equals 3
*/

#include "../../CSPSpec.h"

#include "../../constraints/constraint_abstract.h"

template<typename T>
struct SparseBoundVarContainer;

template<typename DomType = DomainInt>
struct SparseBoundVarRef_internal
{
  static const BOOL isBool = true;
  static const BoundType isBoundConst = Bound_Yes;
  static string name() { return "SparseBound"; }
  BOOL isBound() const
  { return true;}
  
  int var_num;

  
#ifdef MANY_VAR_CONTAINERS
  SparseBoundVarContainer<DomType>* sparseCon;
  SparseBoundVarContainer<DomType>& getCon() const { return *sparseCon; }
  SparseBoundVarRef_internal() : var_num(-1), sparseCon(NULL)
  { }

  explicit SparseBoundVarRef_internal(SparseBoundVarContainer<DomType>* con, int i) : 
  var_num(i), sparseCon(con)
  { }
  
#else
  static SparseBoundVarContainer<DomType>& getCon_Static();
  SparseBoundVarRef_internal() : var_num(-1)
  { }

  explicit SparseBoundVarRef_internal(SparseBoundVarContainer<DomType>*, int i) : 
  var_num(i)
  { }
#endif
};

#ifdef MORE_SEARCH_INFO
typedef InfoRefType<VarRefType<SparseBoundVarRef_internal<> >, VAR_INFO_SPARSEBOUND> SparseBoundVarRef;
#else
typedef VarRefType<SparseBoundVarRef_internal<> > SparseBoundVarRef;
#endif

template<typename BoundType = DomainInt>
struct SparseBoundVarContainer {
  StateObj* stateObj;
  MoveablePointer bound_data;
  TriggerList trigger_list;
  vector<vector<BoundType> > domains;
  vector<int> domain_reference;
  vector<vector<AbstractConstraint*> > constraints;
#ifdef WDEG
  vector<unsigned int> wdegs;
#endif
  unsigned var_count_m;
  BOOL lock_m;

  SparseBoundVarContainer(StateObj* _stateObj) : stateObj(_stateObj), trigger_list(stateObj, true), var_count_m(0), lock_m(false)
  { }
  
  vector<BoundType>& get_domain(SparseBoundVarRef_internal<BoundType> i)
  { return domains[domain_reference[i.var_num]]; }
 
  vector<BoundType>& get_domain_from_int(int i)
  { return domains[domain_reference[i]]; }
  
  const BoundType& lower_bound(SparseBoundVarRef_internal<BoundType> i) const
  { return static_cast<const BoundType*>(bound_data.get_ptr())[i.var_num*2]; }
  
  const BoundType& upper_bound(SparseBoundVarRef_internal<BoundType> i) const
  { return static_cast<const BoundType*>(bound_data.get_ptr())[i.var_num*2 + 1]; }
  
  BoundType& lower_bound(SparseBoundVarRef_internal<BoundType> i)
  { return static_cast<BoundType*>(bound_data.get_ptr())[i.var_num*2]; }
  
  BoundType& upper_bound(SparseBoundVarRef_internal<BoundType> i)
  { return static_cast<BoundType*>(bound_data.get_ptr())[i.var_num*2 + 1]; }
  
  /// find the small possible lower bound above new_lower_bound.
  /// Does not actually change the lower bound.  
  DomainInt find_lower_bound(SparseBoundVarRef_internal<BoundType> d, DomainInt new_lower_bound)
  {
    vector<BoundType>& bounds = get_domain(d);
    typename vector<BoundType>::iterator it = std::lower_bound(bounds.begin(), bounds.end(), new_lower_bound);
    if(it == bounds.end())
    {
      getState(stateObj).setFailed(true);
      return *(it - 1);
    }
    
    return *it;
  }
  
  /// find the largest possible upper bound below new_upper_bound.
  /// Does not actually change the upper bound.
  DomainInt find_upper_bound(SparseBoundVarRef_internal<BoundType>& d, DomainInt new_upper_bound)
  {
    vector<BoundType>& bounds = get_domain(d);

    typename vector<BoundType>::iterator it = std::lower_bound(bounds.begin(), bounds.end(), new_upper_bound);
    if(it == bounds.end())
      return *(it - 1);
    
    if(*it == new_upper_bound)
      return new_upper_bound;
    
    if(it == bounds.begin())
    {
      getState(stateObj).setFailed(true);
      return bounds.front();
    }
    
    return *(it - 1);
  }
  
 void lock()
  { 
    D_ASSERT(!lock_m);
    lock_m = true;
 }

  void addVariables(const vector<pair<int, vector<int> > >& new_domains)
{
  D_ASSERT(!lock_m);
  
  if(new_domains.empty())
  {
    trigger_list.lock(0,0,0);
    return;
  }

  DomainInt min_domain_val = DomainInt_Min;
  DomainInt max_domain_val = DomainInt_Max;

  for(int i = 0; i < new_domains.size(); ++i)
  {
    D_ASSERT(new_domains[i].second.front() >= DomainInt_Min);
    D_ASSERT(new_domains[i].second.back() <= DomainInt_Max);
    
    for(int loop=0;loop<(int)(new_domains[i].second.size()) - 1; ++loop) 
    { D_ASSERT(new_domains[i].second[loop] < new_domains[i].second[loop+1]); }
  
    vector<BoundType> t_dom(new_domains[i].second.size());
    for(unsigned int j = 0; j < new_domains[i].second.size(); ++j)
      t_dom[j] = new_domains[i].second[j];
          
    domains.push_back(t_dom);
    for(int j = 0; j < new_domains[i].first; ++j)
      domain_reference.push_back(i);

    min_domain_val = mymin(t_dom.front(), min_domain_val);
    max_domain_val = mymax(t_dom.back(), max_domain_val);
  }

  // TODO: Setting var_count_m to avoid changing other code.. long term, do
  // we need it?
  var_count_m = domain_reference.size();

  constraints.resize(var_count_m);
#ifdef WDEG
  if(getOptions(stateObj).wdeg_on) wdegs.resize(var_count_m);
#endif

  bound_data = getMemory(stateObj).backTrack().request_bytes(var_count_m*2*sizeof(BoundType));
  BoundType* bound_ptr = static_cast<BoundType*>(bound_data.get_ptr());
  for(unsigned int i = 0; i < var_count_m; ++i)
  {
    bound_ptr[2*i] = get_domain_from_int(i).front();
    bound_ptr[2*i+1] = get_domain_from_int(i).back();
  }
    
  trigger_list.lock(var_count_m, min_domain_val, max_domain_val);
}
  
  BOOL isAssigned(SparseBoundVarRef_internal<BoundType> d) const
  { 
    D_ASSERT(lock_m);
    return lower_bound(d) == upper_bound(d); 
  }
  
  DomainInt getAssignedValue(SparseBoundVarRef_internal<BoundType> d) const
  {
    D_ASSERT(lock_m);
    D_ASSERT(isAssigned(d));
    return lower_bound(d);
  }
  
  BOOL inDomain(SparseBoundVarRef_internal<BoundType> d, DomainInt i) const
  { 
      D_ASSERT(lock_m);
      // First check against bounds
      if(i< lower_bound(d) || i> upper_bound(d))
      {
          return false;
      }
      else
      {
          return inDomain_noBoundCheck(d, i); 
      }
  }
  
  BOOL inDomain_noBoundCheck(SparseBoundVarRef_internal<BoundType> ref, DomainInt i) const
  {
      D_ASSERT(lock_m);
      // use binary search to find if the value is in the domain vector.
      //const vector<BoundType>& dom = get_domain(ref);  // why does this not work?
      const vector<BoundType>& dom = domains[domain_reference[ref.var_num]];
      
      return std::binary_search( dom.begin(), dom.end(), i );
  }
  
  DomainInt getMin(SparseBoundVarRef_internal<BoundType> d) const
  {
    D_ASSERT(lock_m);
    return lower_bound(d);
  }
  
  DomainInt getMax(SparseBoundVarRef_internal<BoundType> d) const
  {
    D_ASSERT(lock_m);
    return upper_bound(d);
  }

  DomainInt getInitialMin(SparseBoundVarRef_internal<BoundType> d)
  { return get_domain_from_int(d.var_num).front(); }
  
  DomainInt getInitialMax(SparseBoundVarRef_internal<BoundType> d)
  { return get_domain_from_int(d.var_num).back(); }
  
  /// This function is provided for convience. It should never be called.
  void removeFromDomain(SparseBoundVarRef_internal<BoundType>, DomainInt)
  {
    D_FATAL_ERROR("Cannot Remove Value from domain of a bound var");
    FAIL_EXIT();
  }

  void internalAssign(SparseBoundVarRef_internal<BoundType> d, DomainInt i)
  {
    vector<BoundType>& bounds = get_domain(d);
    DomainInt min_val = getMin(d);
    DomainInt max_val = getMax(d);

    if(!binary_search(bounds.begin(), bounds.end(), i))
    {
      getState(stateObj).setFailed(true);
      return;
    }
    if(min_val > i || max_val < i)
    {
      getState(stateObj).setFailed(true);
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
  
  void propagateAssign(SparseBoundVarRef_internal<BoundType> d, DomainInt i)
  { internalAssign(d, i); }
  
  // TODO : Optimise
  void uncheckedAssign(SparseBoundVarRef_internal<BoundType> d, DomainInt i)
  { internalAssign(d, i); }

  void decisionAssign(SparseBoundVarRef_internal<BoundType> d, DomainInt i)
  { internalAssign(d, i); }
  
  void setMax(SparseBoundVarRef_internal<BoundType> d, DomainInt i)
  {
    // Note, this just finds a new upper bound, it doesn't set it.
    i = find_upper_bound(d, i);
    
    DomainInt low_bound = lower_bound(d);
    
    if(i < low_bound)
    {
      getState(stateObj).setFailed(true);
      return;
    }
    
    DomainInt up_bound = upper_bound(d);
    
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
  
  void setMin(SparseBoundVarRef_internal<BoundType> d, DomainInt i)
  {
    i = find_lower_bound(d,i);
    
    DomainInt up_bound = upper_bound(d);
    
    if(i > up_bound)
    {
      getState(stateObj).setFailed(true);
      return;
    }
    
    DomainInt low_bound = lower_bound(d);
    
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

  void addTrigger(SparseBoundVarRef_internal<BoundType> b, Trigger t, TrigType type)
  { 
    D_ASSERT(lock_m); 
	trigger_list.add_trigger(b.var_num, t, type); 
  }

  vector<AbstractConstraint*>* getConstraints(const SparseBoundVarRef_internal<BoundType>& b)
  { return &constraints[b.var_num]; }

  void addConstraint(const SparseBoundVarRef_internal<BoundType>& b, AbstractConstraint* c)
  { 
    constraints[b.var_num].push_back(c); 
#ifdef WDEG
    if(getOptions(stateObj).wdeg_on) wdegs[b.var_num] += c->getWdeg(); //add constraint score to base var wdeg
#endif
  }
  
  DomainInt getBaseVal(const SparseBoundVarRef_internal<BoundType>& b, DomainInt v) const 
  { 
    D_ASSERT(inDomain(b, v));
    return v; 
  }

  Var getBaseVar(const SparseBoundVarRef_internal<BoundType>& b) const
  { return Var(VAR_SPARSEBOUND, b.var_num); }

#ifdef WDEG
  int getBaseWdeg(const SparseBoundVarRef_internal<BoundType>& b)
  { return wdegs[b.var_num]; }

  void incWdeg(const SparseBoundVarRef_internal<BoundType>& b)
  { wdegs[b.var_num]++; }
#endif

#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(SparseBoundVarRef_internal<BoundType> b, DynamicTrigger* t, TrigType type, DomainInt pos = -999)
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
      if(!isAssigned(SparseBoundVarRef_internal<BoundType>(i)))
    s << "X";
      else
      {
    s << (getAssignedValue(SparseBoundVarRef_internal<BoundType>(i))?1:0); 
      }
      char_count++;
      if(char_count%7==0) s << endl;
    }
    return s.str();
  }
  
};


template<typename T>
inline SparseBoundVarRef
SparseBoundVarContainer<T>::get_var_num(int i)
{
  D_ASSERT(i < var_count_m);
  return SparseBoundVarRef(SparseBoundVarRef_internal<T>(this, i));
}

