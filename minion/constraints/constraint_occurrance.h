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

/** @help constraints;occurrence Description
The constraint

   occurrence(vec, elem, count)

ensures that there are count occurrences of the value elem in the
vector vec.
*/

/** @help constraints;occurrence Notes
elem must be a constant, not a variable.
*/

/** @help constraints;occurrence Reifiability
This constraint is not reifiable.
*/

/** @help constraints;occurrence References
help constraints occurrenceleq
help constraints occurrencegeq
*/

/** @help constraints;occurrenceleq Description
The constraint

   occurrenceleq(vec, elem, count)

ensures that there are AT MOST count occurrences of the value elem in
the vector vec.
*/

/** @help constraints;occurrenceleq Notes
elem must be a constant
*/

/** @help constraints;occurrenceleq Reifiability
This constraint is not reifiable.
*/

/** @help constraints;occurrenceleq References
help constraints occurrence
help constraints occurrencegeq
*/

/** @help constraints;occurrencegeq Description
The constraint

   occurrencegeq(vec, elem, count)

ensures that there are AT LEAST count occurrences of the value elem in
the vector vec.
*/

/** @help constraints;occurrencegeq Notes
elem must be a constant
*/

/** @help constraints;occurrencegeq Reifiability
This constraint is not reifiable.
*/

/** @help constraints;occurrencegeq References
help constraints occurrence
help constraints occurrenceleq
*/

template<typename VarArray, typename Val>
struct ConstantOccurrenceEqualConstraint : public Constraint
{
  virtual string constraint_name()
  { return "OccurrenceLeq/Geq"; }
  
  typedef typename VarArray::value_type VarRef;
  
  ReversibleInt occurrences_count;
  ReversibleInt not_occurrences_count;
  VarArray var_array;
  
  int val_count_min;
  int val_count_max;
  Val value;
  
  ConstantOccurrenceEqualConstraint(StateObj* _stateObj, const VarArray& _var_array, const Val& _value, 
                            int _val_count_min, int _val_count_max) :
    Constraint(_stateObj), occurrences_count(_stateObj), not_occurrences_count(_stateObj),
    var_array(_var_array), val_count_min(_val_count_min), val_count_max(_val_count_max), value(_value)
  { }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
    occurrences_count = 0; 
    not_occurrences_count = 0;
    for(unsigned int i=0; i < var_array.size(); ++i)
      t.push_back(make_trigger(var_array[i], Trigger(this, i), Assigned));
    return t;
  }
  
  void occurrence_limit_reached()
  {
    D_INFO(1,DI_SUMCON,"Occurrence Limit Reached");
    D_ASSERT(val_count_max <= occurrences_count);
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
    if(val_count_max < occs) 
      getState(stateObj).setFailed(true);
  }
  
  void not_occurrence_limit_reached()
  {
    D_INFO(1,DI_SUMCON,"Not Occurrence Limit Reached");
    D_ASSERT(not_occurrences_count >= static_cast<int>(var_array.size()) - val_count_min);
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
    if(val_count_min > static_cast<int>(var_array.size()) - occs)
      getState(stateObj).setFailed(true);
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
	  PROP_INFO_ADDONE(OccEqual);
    D_ASSERT(i >= 0);
    
    if( var_array[i].getAssignedValue() == value )
    {
      ++occurrences_count;
      if(val_count_max < occurrences_count)
        getState(stateObj).setFailed(true);
      if(occurrences_count == val_count_max)
        occurrence_limit_reached();
    }
    else
    {
      ++not_occurrences_count;
      if(val_count_min > static_cast<int>(var_array.size()) - not_occurrences_count)
        getState(stateObj).setFailed(true);
      if(not_occurrences_count == static_cast<int>(var_array.size()) - val_count_min )
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
    D_INFO(1, DI_SUMCON, "Start full propagate");
    if(val_count_max < 0 || val_count_min > (int)var_array.size())
      getState(stateObj).setFailed(true);
    setup_counters();
    D_INFO(1,DI_SUMCON,to_string("Full Propagate, count", occurrences_count));
    
    if(val_count_max < occurrences_count)
      getState(stateObj).setFailed(true);

    if(val_count_min > static_cast<int>(var_array.size()) - not_occurrences_count)
      getState(stateObj).setFailed(true);
        
    if(occurrences_count == val_count_max)
      occurrence_limit_reached();
    if(not_occurrences_count == static_cast<int>(var_array.size()) - val_count_min)
      not_occurrence_limit_reached();
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array.size());
    DomainInt count = 0;  
    for(int i = 0; i < v_size; ++i)
      count += (*(v + i) == value);
    return (count >= val_count_min) && (count <= val_count_max);
  }
  
  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
	  vars.reserve(var_array.size());
	  for(unsigned i = 0; i < var_array.size(); ++i)
	    vars.push_back(AnyVarRef(var_array[i]));
	  return vars;
  }
};

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
    val_count.setMin(0);
    val_count.setMax(var_array.size());
    setup_counters();
    D_INFO(1,DI_SUMCON,to_string("Full Propagate, count", occurrences_count));
    val_count.setMin(occurrences_count);
    val_count.setMax(static_cast<int>(var_array.size()) - not_occurrences_count);
    
    if(occurrences_count == val_count.getMax())
      occurrence_limit_reached();
    if(not_occurrences_count == static_cast<int>(var_array.size()) - val_count.getMin() )
      not_occurrence_limit_reached();
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array.size() + 1);
    DomainInt count = 0;  
    for(int i = 0; i < v_size - 1; ++i)
      count += (*(v + i) == value);
    return count == *(v + v_size - 1);
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

template<typename VarArray, typename Val>
Constraint*
ConstantOccEqualCon(StateObj* stateObj, const VarArray& _var_array,  const Val& _value, int _val_count_min, int _val_count_max)
{ 
  return 
  (new ConstantOccurrenceEqualConstraint<VarArray,Val>(stateObj, _var_array,  _value, _val_count_min, _val_count_max)); 
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
    { return reifyCon(stateObj, ConstantOccEqualCon(stateObj, t1, runtime_val(val_to_count), 0, occs), reifyVar); } 
  else 
    { return ConstantOccEqualCon(stateObj, t1, runtime_val(val_to_count), 0, occs); } 
}

template<typename T1>
Constraint*
BuildCT_GEQ_OCCURRENCE(StateObj* stateObj, const T1& t1, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b) 
{
  int val_to_count = b.vars[1][0].pos;
  int occs = b.vars[2][0].pos;
  if(reify) 
  { return reifyCon(stateObj, ConstantOccEqualCon(stateObj, t1, runtime_val(val_to_count), occs, t1.size()), reifyVar); } 
  else 
  { return ConstantOccEqualCon(stateObj, t1, runtime_val(val_to_count), occs, t1.size()); } 
}
