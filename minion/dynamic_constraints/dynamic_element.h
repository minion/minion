/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
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

/** @help constraints;watchelement_one Description
This constraint is identical to watchelement, except the vector
is indexed from 1 rather than from 0.
*/

/** @help constraints;watchelement_one References
See entry

   help constraints watchelement

for details of watchelement which watchelement_one is based on.
*/

/** @help constraints;watchelement Description
The constraint 

   watchelement(vec, i, e)

specifies that, in any solution, vec[i] = e and i is in the range 
[0 .. |vec|-1].
*/

/** @help constraints;watchelement Notes
Enforces generalised arc consistency.
*/

/** @help constraints;watchelement References
See entry

   help constraints element

for details of an identical constraint that enforces a lower level of
consistency.
*/

// The triggers in this constraint are set up as follows:
// If the length of the vector is L.

// The first 2 * Dom(Result) literals are, for some j
//   literal 2 * i : attached to assignment i to V[j]
//   literal 2 * i + 1 : attached to the assignment j in IndexVar 

// After this there are 2 * Dom(Index) literals are, for some j
// literal 2 * i : attached to j in V[i]
// literal 2 * i + 1 : attached to j in Result

#ifndef CONSTRAINT_DYNAMIC_ELEMENT_H
#define CONSTRAINT_DYNAMIC_ELEMENT_H

// for the reverse constraint.
#include "../constraints/constraint_equal.h"
#include "dynamic_new_or.h"
#include "dynamic_new_and.h"
#include "unary/dynamic_literal.h"
#include "unary/dynamic_notinrange.h"


template<typename VarArray, typename Index, typename Result>
struct ElementConstraintDynamic : public AbstractConstraint
{
  virtual string constraint_name()
  { return "ElementDynamic"; }
  
  //  typedef BoolLessSumConstraintDynamic<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;
  
  // ReversibleInt count;
  VarArray var_array;
  Index indexvar;
  Result resultvar;
  
  DomainInt initial_result_dom_min;
  DomainInt initial_result_dom_max;
  
  vector<DomainInt> current_support;
  
  ElementConstraintDynamic(StateObj* _stateObj, const VarArray& _var_array, const Index& _index, const Result& _result) :
    AbstractConstraint(_stateObj), var_array(_var_array), indexvar(_index), resultvar(_result)
  { 
      initial_result_dom_min = resultvar.getInitialMin();
      initial_result_dom_max = resultvar.getInitialMax();
  }
  
  int dynamic_trigger_count()
  {
    int count = var_array.size() * 2 + 
    checked_cast<int>(initial_result_dom_max - initial_result_dom_min + 1) * 2 
    + 1 
    + 1; 
    current_support.resize(count / 2);           // is int the right type?
    return count;
  }
  
  void find_new_support_for_result(int j)
  {
    DomainInt realj = j + initial_result_dom_min;
    
    if(!resultvar.inDomain(realj))
      return;
    
    int array_size = var_array.size();
    
    // support is value of index
    DomainInt oldsupport = max(current_support[j + array_size], indexvar.getMin());  // old support probably just removed
    DomainInt maxsupport = indexvar.getMax();
    
    DomainInt support = oldsupport;
    
    DynamicTrigger* dt = dynamic_trigger_start();
    while(support <= maxsupport && 
          !(indexvar.inDomain_noBoundCheck(support) && 
            var_array[checked_cast<int>(support)].inDomain(realj)))
      ++support;
    if(support > maxsupport)
    { 
      support = indexvar.getMin();
      DomainInt max_check = min(oldsupport, maxsupport + 1);
      while(support < max_check && 
            !(indexvar.inDomain_noBoundCheck(support) &&
              var_array[checked_cast<int>(support)].inDomain(realj)))
        ++support;
      if (support == max_check) 
      {
        resultvar.removeFromDomain(realj); 
        return;
      }
    }
    var_array[checked_cast<int>(support)].addDynamicTrigger(dt + 2*j, DomainRemoval, realj);
    indexvar.addDynamicTrigger(dt + 2*j + 1, DomainRemoval, support);
    current_support[j + array_size] = support;
  }
  
  void find_new_support_for_index(int i)
  {
    if(!indexvar.inDomain(i))
      return;
    
    DomainInt resultvarmin = resultvar.getMin();
    DomainInt resultvarmax = resultvar.getMax();
    DynamicTrigger* dt = dynamic_trigger_start() + 
                         checked_cast<int>((initial_result_dom_max - initial_result_dom_min + 1) * 2);
                         
    if(resultvarmin == resultvarmax)
    {
      if(!var_array[i].inDomain(resultvarmin))
        indexvar.removeFromDomain(i);
      else
      {
        var_array[i].addDynamicTrigger(dt + 2*i, DomainRemoval, resultvarmin);
        resultvar.addDynamicTrigger(dt + 2*i + 1, DomainRemoval, resultvarmin);
        current_support[i] = resultvarmin;
      }
      return;
    }
    

    // support is value of result
    DomainInt oldsupport = max(current_support[i], resultvarmin); // old support probably just removed
    DomainInt maxsupport = resultvarmax;
    DomainInt support = oldsupport;
    
    //int support = initial_result_dom_min;
    while(support <= maxsupport &&
          !(resultvar.inDomain_noBoundCheck(support) && var_array[i].inDomain(support)))
      ++support;
      
    if(support > maxsupport)
    { 
      support = resultvarmin;
      DomainInt max_check = min(oldsupport, maxsupport + 1);
      while(support < max_check &&     
            !(resultvar.inDomain_noBoundCheck(support) && var_array[i].inDomain(support)))
        ++support;
      if( support >= max_check )      
      {
        indexvar.removeFromDomain(i); 
        return;
      }
    }
    
    var_array[i].addDynamicTrigger(dt + 2*i, DomainRemoval, support);
    resultvar.addDynamicTrigger(dt + 2*i + 1, DomainRemoval, support);
    current_support[i] = support;
  }
  
  void deal_with_assigned_index()
  {
    D_ASSERT(indexvar.isAssigned());
    int indexval = checked_cast<int>(indexvar.getAssignedValue());
    VarRef& var = var_array[indexval];
    
    DomainInt lower = resultvar.getMin(); 
    if( lower > var.getMin() ) 
    {
      var.setMin(lower);
      ++lower;                      // do not need to check lower bound, we know it's in resultvar
    }
    
    DomainInt upper = resultvar.getMax(); 
    if( upper < var.getMax() ) 
    {
      var.setMax(upper);
      --upper;                      // do not need to check upper bound, we know it's in resultvar
    }
    
    for(DomainInt i = lower; i <= upper; ++i)
    {
      if(!(resultvar.inDomain(i)))
        var.removeFromDomain(i); 
    }
  }
  
  virtual void full_propagate()
  {
    for(int i=0; i<var_array.size(); i++) {
        if(var_array[i].isBound() && !var_array[i].isAssigned()) { // isassigned excludes constants.
            cerr << "Warning: watchelement is not designed to be used on bound variables and may cause crashes." << endl;
        }
    }
    if((indexvar.isBound() && !indexvar.isAssigned())
        || (resultvar.isBound() && !resultvar.isAssigned())) {
        cerr << "Warning: watchelement is not designed to be used on bound variables and may cause crashes." << endl;
    }
    
    int array_size = var_array.size(); 
    DomainInt result_dom_size = initial_result_dom_max - initial_result_dom_min + 1;
    
    // Setup SupportLostForIndexValue(i,j)
    // Here we are supporting values in the index variable
    // So for each variable in the index variable, we want to ensure
    
    // Couple of quick sanity-propagations.
    // We define UNDEF = false ;)
    indexvar.setMin(0);
    indexvar.setMax(array_size - 1);
    
    if(getState(stateObj).isFailed()) return;
    
    for(int i = 0; i < array_size; ++i)
    {
      current_support[i] = initial_result_dom_min-1;        // will be incremented if support sought
      if(indexvar.inDomain(i))
        find_new_support_for_index(i);
    }
    
    for(int i = 0; i < result_dom_size; ++i)
    {
      current_support[i+array_size] = -1;   // will be incremented if support sought
      if(resultvar.inDomain(i + initial_result_dom_min))
        find_new_support_for_result(i);
    }
    
    if(indexvar.isAssigned())
      deal_with_assigned_index();
    
    DynamicTrigger* dt = dynamic_trigger_start();
    
    dt += var_array.size() * 2 +
      checked_cast<int>((initial_result_dom_max - initial_result_dom_min + 1) * 2);
    
    // for(int i = initial_result_dom_min; i <= initial_result_dom_max; ++i)
    // {
    // resultvar.addDynamicTrigger(dt, DomainRemoval, i);
    // ++dt;
    // }
    resultvar.addDynamicTrigger(dt, DomainChanged);  // Why is this always here-- why not place it when indexvar becomes assigned, lift it 
    // whenever it triggers when indexvar is not assigned.
    ++dt;
    
    indexvar.addDynamicTrigger(dt, Assigned);
  }
  
  
  virtual void propagate(DynamicTrigger* trig)
  {
    PROP_INFO_ADDONE(DynElement);
    DynamicTrigger* dt = dynamic_trigger_start();
    unsigned pos = trig - dt;
    unsigned array_size = var_array.size();
    unsigned result_support_triggers = 
      checked_cast<unsigned int>((initial_result_dom_max - initial_result_dom_min + 1) * 2);
    unsigned index_support_triggers =  array_size * 2;
    // int when_index_assigned_triggers = (initial_result_dom_max - initial_result_dom_min + 1);
    if(pos < result_support_triggers)
    {// It was a value in the result var which lost support
      find_new_support_for_result(pos / 2);
      return;
    }
    pos -= result_support_triggers;
    
    if(pos < index_support_triggers)
    {// A value in the index var lost support
      find_new_support_for_index( pos / 2 );
      return;
    }
    pos -= index_support_triggers;
    
    // if(pos < when_index_assigned_triggers)
    if (pos == 0)
    { // A value was removed from result var
      if(indexvar.isAssigned())
      {
        deal_with_assigned_index();
      }
      return;
    }
    
    D_ASSERT(pos == 1);
    // index has become assigned.
    deal_with_assigned_index();
  }
  
    virtual BOOL check_assignment(DomainInt* v, int v_size)
    {
      D_ASSERT(v_size == var_array.size() + 2);
      DomainInt resultvariable = v[v_size - 1];
      DomainInt indexvariable = v[v_size - 2];
      if(indexvariable < 0 || indexvariable >= (int)v_size - 2)
        return false;
      return v[checked_cast<int>(indexvariable)] == resultvariable;
    }
    
    virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size() + 2);
    for(unsigned i = 0; i < var_array.size(); ++i)
      vars.push_back(var_array[i]);
    vars.push_back(indexvar);
    vars.push_back(resultvar);
    return vars;
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {  
    DomainInt array_start = max(DomainInt(0), indexvar.getMin());
    DomainInt array_end   = min(DomainInt(var_array.size()) - 1, indexvar.getMax());

    for(int i = array_start; i <= array_end; ++i)
    {
      if(indexvar.inDomain(i))
      {
        DomainInt dom_start = max(resultvar.getMin(), var_array[i].getMin());
        DomainInt dom_end   = min(resultvar.getMax(), var_array[i].getMax());
        for(DomainInt domval = dom_start; domval <= dom_end; ++domval)
        {
          if(var_array[i].inDomain(domval) && resultvar.inDomain(domval))
          {
            // indexvar = i
            assignment.push_back(make_pair(var_array.size(), i));
            // resultvar = domval
            assignment.push_back(make_pair(var_array.size() + 1, domval));
            // vararray[i] = domval
            assignment.push_back(make_pair(i, domval));
            return true;
          }
        }
      }
    }
    return false;
  }
  
  virtual AbstractConstraint* reverse_constraint()
  {
      // This is a slow-ish temporary solution.
      // (i=1 and X[1]!=r) or (i=2 ...
      vector<AbstractConstraint*> con;
      // or the index is out of range:
      vector<int> r; r.push_back(0); r.push_back(var_array.size()-1);
      AbstractConstraint* t4=(AbstractConstraint*) new WatchNotInRangeConstraint<Index>(stateObj, indexvar, r);
      con.push_back(t4);
      
      for(int i=0; i<var_array.size(); i++)
      {
          vector<AbstractConstraint*> con2;
          WatchLiteralConstraint<Index>* t=new WatchLiteralConstraint<Index>(stateObj, indexvar, i);
          con2.push_back((AbstractConstraint*) t);
          NeqConstraintBinary<AnyVarRef, Result>* t2=new NeqConstraintBinary<AnyVarRef, Result>(stateObj, var_array[i], resultvar);
          con2.push_back((AbstractConstraint*) t2);
          
          Dynamic_AND* t3= new Dynamic_AND(stateObj, con2);
          con.push_back((AbstractConstraint*) t3);
      }
      
      return new Dynamic_OR(stateObj, con);
  }
};

#endif
