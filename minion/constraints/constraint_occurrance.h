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

template<typename VarArray, typename Val, typename ValCount>
struct OccurrenceEqualConstraint : public Constraint
{
  virtual string constraint_name()
  { return "OccurrenceEqual"; }
  
  typedef typename VarArray::value_type VarRef;
  
  ReversibleInt occurrences_count;
  ReversibleInt not_occurrences_count;
  VarArray var_array;
  
  ValCount val_count;
  Val value;
  
  OccurrenceEqualConstraint(StateObj* _stateObj, const VarArray& _var_array, const Val& _value, const ValCount& _val_count) :
    Constraint(_stateObj), occurrences_count(_stateObj), not_occurrences_count(_stateObj),
    var_array(_var_array), val_count(_val_count), value(_value)
  { }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
    occurrences_count = 0; 
    not_occurrences_count = 0;
    for(unsigned int i=0; i < var_array.size(); ++i)
      t.push_back(make_trigger(var_array[i], Trigger(this, i), Assigned));
    t.push_back(make_trigger(val_count, Trigger(this, -1), UpperBound));
    t.push_back(make_trigger(val_count, Trigger(this, -2), LowerBound));
    return t;
  }
  
  void occurrence_limit_reached()
  {
    D_INFO(1,DI_SUMCON,"Occurrence Limit Reached");
    D_ASSERT(val_count.getMax() <= occurrences_count);
    int occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it=var_array.begin(); it < end_it; ++it)
    {
      if(it->isAssigned())
      { 
        if(it->getAssignedValue() == value) 
        ++occs;
      }
      else
      { 
        it->removeFromDomain(value);
      }
    }
    val_count.setMin(occs);
  }
  
  void not_occurrence_limit_reached()
  {
    D_INFO(1,DI_SUMCON,"Not Occurrence Limit Reached");
    D_ASSERT(not_occurrences_count >= static_cast<int>(var_array.size()) - val_count.getMin());
    int occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for( typename VarArray::iterator it=var_array.begin(); it < end_it; ++it)
    {
      if(it->isAssigned())
      { 
      if(it->getAssignedValue() != value) 
        ++occs; 
      }
      else
      { it->propagateAssign(value); }
    }
    val_count.setMax(static_cast<int>(var_array.size()) - occs);
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
	PROP_INFO_ADDONE(OccEqual);
    if(i < 0)
    { // val_count changed
      if(occurrences_count == val_count.getMax())
        occurrence_limit_reached();
      if(not_occurrences_count == static_cast<int>(var_array.size()) - val_count.getMin() )
        not_occurrence_limit_reached();
      return;
    }
    
    if( var_array[i].getAssignedValue() == value )
    {
      ++occurrences_count;
      val_count.setMin(occurrences_count);
      if(occurrences_count == val_count.getMax())
        occurrence_limit_reached();
    }
    else
    {
      ++not_occurrences_count;
      val_count.setMax(static_cast<int>(var_array.size()) - not_occurrences_count);
      if(not_occurrences_count == static_cast<int>(var_array.size()) - val_count.getMin() )
        not_occurrence_limit_reached();
    }
  }
  
  void setup_counters()
  {
    int occs = 0;
	int not_occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it=var_array.begin(); it < end_it; ++it)
    {
      if(it->isAssigned())
	  {
	    if(it->getAssignedValue() == value)
	      ++occs;
		else
	      ++not_occs;
      }
    }
    occurrences_count = occs;
	not_occurrences_count = not_occs;
  }
  
  virtual void full_propagate()
  {
    setup_counters();
    int i = occurrences_count;
    int j = not_occurrences_count;
    D_INFO(1,DI_SUMCON,to_string("Full Propagate, count",i));
    val_count.setMin(occurrences_count);
    val_count.setMax(static_cast<int>(var_array.size()) - not_occurrences_count);
    
    if(occurrences_count == val_count.getMax())
      occurrence_limit_reached();
    if(not_occurrences_count == static_cast<int>(var_array.size()) - val_count.getMin() )
      not_occurrence_limit_reached();
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    D_ASSERT(v.size() == var_array.size() + 1);
    DomainInt count = 0;  
    typename vector<DomainInt>::iterator end_it(v.end());
    end_it--;
    for( typename vector<DomainInt>::iterator it=v.begin(); it < end_it; ++it)
      count += (*it == value);
    return count == v.back();
  }
  
  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
	vars.reserve(var_array.size() + 1);
	for(unsigned i = 0; i < var_array.size(); ++i)
	  vars.push_back(AnyVarRef(var_array[i]));
    vars.push_back(AnyVarRef(val_count));
	return vars;
  }
};



template<typename VarArray, typename Val, typename ValCount>
Constraint*
OccEqualCon(StateObj* stateObj, const VarArray& _var_array,  const Val& _value, const ValCount& _val_count)
{ 
  return 
  (new OccurrenceEqualConstraint<VarArray,Val, ValCount>(stateObj, _var_array,  _value, _val_count)); 
}

template<typename T1, typename T2, typename T3>
Constraint*
BuildCT_OCCURRENCE(StateObj* stateObj, const T1& t1, const T2& t2, const T3& t3, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b) 
{
  int val_to_count = b.vars[1][0].pos;
//  int occs = b.vars[2][0].pos;
  if(reify) 
  { return reifyCon(stateObj, OccEqualCon(stateObj, t1, runtime_val(val_to_count), t3[0]), reifyVar); } 
  else 
  { return OccEqualCon(stateObj, t1, runtime_val(val_to_count), t3[0]); } 
}

template<typename T1>
Constraint*
BuildCT_LEQ_OCCURRENCE(StateObj* stateObj, const T1& t1, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b) 
{
  int val_to_count = b.vars[1][0].pos;
  int occs = b.vars[2][0].pos;
  if(reify) 
    { return reifyCon(stateObj, OccEqualCon(stateObj, t1, runtime_val(val_to_count), TrivialBoundVar(stateObj,0,occs)), reifyVar); } 
  else 
    { return OccEqualCon(stateObj, t1, runtime_val(val_to_count), TrivialBoundVar(stateObj,0,occs)); } 
}

template<typename T1>
Constraint*
BuildCT_GEQ_OCCURRENCE(StateObj* stateObj, const T1& t1, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b) 
{
  int val_to_count = b.vars[1][0].pos;
  int occs = b.vars[2][0].pos;
  if(reify) 
  { return reifyCon(stateObj, OccEqualCon(stateObj, t1, runtime_val(val_to_count), TrivialBoundVar(stateObj,occs, t1.size())), reifyVar); } 
  else 
  { return OccEqualCon(stateObj, t1, runtime_val(val_to_count), TrivialBoundVar(stateObj,occs, t1.size())); } 
}
