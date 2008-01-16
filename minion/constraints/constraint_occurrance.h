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
  
  // typedef BoolLessSumConstraint<VarArray, Val, runtime_val> NegConstraintType;
  typedef typename VarArray::value_type VarRef;
  
  ReversibleInt occurrences_count;
  ReversibleInt not_occurrences_count;
  VarArray var_array;
  
  ValCount val_count;
  Val value;
  
  OccurrenceEqualConstraint(const VarArray& _var_array, const Val& _value, const ValCount& _val_count) :
    var_array(_var_array), val_count(_val_count), value(_value)
  { }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
    occurrences_count.set(0); 
    not_occurrences_count.set(0);
    for(unsigned int i=0; i < var_array.size(); ++i)
		  t.push_back(make_trigger(var_array[i], Trigger(this, i), Assigned));
    return t;
  }
  
  void occurrence_limit_reached()
  {
    D_INFO(1,DI_SUMCON,"Occurrence Limit Reached");
    int occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for(typename VarArray::iterator it=var_array.begin(); it < end_it; ++it)
    {
      if(it->isAssigned())
      { 
	if(it->getAssignedValue() == value.val()) 
	  ++occs; 
      }
      else
      { it->removeFromDomain(value.val()); }
    }
    //D_ASSERT(occs >= oalc_count());
    if(occs > val_count.val())
      Controller::fail();
  }
  
  void not_occurrence_limit_reached()
  {
    D_INFO(1,DI_SUMCON,"Not Occurrence Limit Reached");
    int occs = 0;
    typename VarArray::iterator end_it(var_array.end());
    for( typename VarArray::iterator it=var_array.begin(); it < end_it; ++it)
    {
      if(it->isAssigned())
      { 
      if(it->getAssignedValue() != value.val()) 
        ++occs; 
      }
      else
      { it->propogateAssign(value.val()); }
    }
    //D_ASSERT(occs >= oalc_count());
    if(occs > (static_cast<int>(var_array.size()) - val_count.val()))
      Controller::fail();
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
    if( var_array[i].getAssignedValue() == value.val() )
    {
      int c = occurrences_count.get() + 1;
      occurrences_count.set(c);
      if(c == val_count.val())
        occurrence_limit_reached();
    }
    else
    {
      int c = not_occurrences_count.get() + 1;
      not_occurrences_count.set(c);
      if(c == (static_cast<int>(var_array.size()) - val_count.val()))
	not_occurrence_limit_reached();
    }
  }
  
  virtual BOOL check_unsat(int i, DomainDelta)
  {
    if( var_array[i].getAssignedValue() == value.val() )
    {
      int c = occurrences_count.get() + 1;
      occurrences_count.set(c);
      if(c > val_count.val())
        return true;    
    }
    else
    {
      int c = not_occurrences_count.get() + 1;
      not_occurrences_count.set(c);
      if(c > (static_cast<int>(var_array.size()) - val_count.val()))
	return true;
    }
    return false;
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
	    if(it->getAssignedValue() == value.val())
	      ++occs;
		else
	      ++not_occs;
      }
    }
    occurrences_count.set(occs);
	not_occurrences_count.set(not_occs);
  }
  
  virtual void full_propogate()
  {
    setup_counters();
    int i = occurrences_count.get();
    int j = not_occurrences_count.get();
    D_INFO(1,DI_SUMCON,to_string("Full Propogate, count",i));
    if(i > val_count.val())
      Controller::fail();
    if(i == val_count.val())
      occurrence_limit_reached(); 
    if(j > (static_cast<int>(var_array.size() - val_count.val())))
       Controller::fail();
    if(j == (static_cast<int>(var_array.size() - val_count.val())))
      not_occurrence_limit_reached();
  }
  
  virtual BOOL check_assignment(vector<int> v)
  {
    D_ASSERT(v.size() == var_array.size());
    int count = 0;  
    typename vector<int>::iterator end_it(v.end());
    for( typename vector<int>::iterator it=v.begin(); it < end_it; ++it)
      count += (*it == value.val());
    return count == val_count.val();
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
Constraint*
OccEqualCon(const VarArray& _var_array,  const Val& _value,const ValCount& _val_count)
{ 
  return 
  (new OccurrenceEqualConstraint<VarArray,Val, ValCount>(_var_array,  _value,_val_count)); 
}

template<typename T1>
Constraint*
BuildCT_OCCURRENCE(const T1& t1, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob& b) 
{
  int val_to_count = b.vars[1][0].pos;
  int occs = b.vars[2][0].pos;
  if(reify) 
  { return reifyCon(OccEqualCon(t1, runtime_val(val_to_count), runtime_val(occs)), reifyVar); } 
  else 
  { return OccEqualCon(t1, runtime_val(val_to_count), runtime_val(occs)); } 
}



// The rest of this file is an implementation of LeqOccurrence. This isn't currently used, and
// probably doesn't work properly. Be warned!

/*
template<typename VarArray, typename Val, typename ValCount>
Constraint*
OccLeqCon(const VarArray& _var_array, const Val& _value, const ValCount& _val_count)
{ 
  return 
  (new OccurrenceLeqConstraint<VarArray,Val, ValCount>(_var_array,  _value, _val_count)); 
}
*/
/*
template<typename VarArray, typename Val, typename ValCount>
struct OccurrenceLeqConstraint : public Constraint
{
  virtual string constraint_name()
  { return "OccurrenceLeq"; }
  
 // typedef BoolLessSumConstraint<VarArray, Val, runtime_val> NegConstraintType;
  typedef typename VarArray::value_type VarRef;
  
  ReversibleInt count;
  VarArray var_array;
  
  ValCount val_count;
  Val value;
  
  OccurrenceLeqConstraint(const VarArray& _var_array, const Val& _value, const ValCount& _val_count) :
    var_array(_var_array), val_count(_val_count), value(_value)
  { D_ASSERT(!(val_count.val() == 0 || val_count.val() == static_cast<int>(var_array.size()))); }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
    int array_size = var_array.size();
    count.set(0);    
	
    for(int i = 0; i < array_size; ++i)
	  t.push_back(make_trigger(var_array[i], Trigger(this, i), Assigned));
    return t;
  }

  void limit_reached()
  {
    D_INFO(1,DI_SUMCON,"Limit Reached");
    int occs = 0;
    typename VarArray::iterator end_it = var_array.end();
    for(typename VarArray::iterator it = var_array.begin(); it < end_it; ++it )
    {
      if(it->isAssigned())
      { 
	    if(it->getAssignedValue() == value.val()) 
	      ++occs; 
      }
      else
      { it->removeFromDomain(value.val()); }
    }
    if(occs > val_count.val())
      Controller::fail();
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
    if( var_array[i].getAssignedValue() == value.val() )
    {
      int c = count.get() + 1;
      count.set(c);
      if(c == val_count.val())
        limit_reached();
    }
  }
  
  virtual BOOL check_unsat(int i, DomainDelta)
  {
    if( var_array[i].getAssignedValue() == value.val() )
    {
    int c = count.get() + 1;
    D_INFO(1,DI_SUMCON,to_string("Checking unsat, count",c));
    count.set(c);
    if(c > val_count.val())
      return true;
    else
      return false;
    }
  }
  
  virtual void full_propogate()
  {
    int i = count.get();
    D_INFO(1,DI_SUMCON,to_string("Full Propogate, count",i));
    if(i > val_count.val())
      Controller::fail();
    if(i == val_count.val())
      limit_reached();  
  }
  
  virtual BOOL check_assignment(vector<int> v)
  {
    D_ASSERT(v.size() == var_array.size());
    int c = 0;  
    for(unsigned int i=0;i<v.size();i++)
      c += (v[i] == value.val());
    return c <= value.val();
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars(var_array.size());
	for(unsigned i = 0; i < var_array.size(); ++i)
	  vars[i] = AnyVarRef(var_array[i]);
	return vars;
  }
};
*/

