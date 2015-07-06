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

/** @help constraints;watchelement_undefzero Description
The constraint

   watchelement_undefzero(vec, i, e)

specifies that, in any solution, either:
a)  vec[i] = e and i is in the range [0 .. |vec|-1]
b)  i is outside the index range of vec, and e = 0

Unlike watchelement (and element) which are false if i is outside
the index range of vec.

In general, use watchelement unless you have a special reason to
use this constraint!

*/

/** @help constraints;watchelement_undefzero Notes
Enforces generalised arc consistency.
*/

/** @help constraints;watchelement_undefzero References
See entry

   help constraints watchelement

for details of the standard element constraint, which is false
when the array value is out of bounds.
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
#include "constraint_equal.h"
#include "dynamic_new_or.h"
#include "dynamic_new_and.h"
#include "unary/dynamic_literal.h"
#include "unary/dynamic_notinrange.h"


template<typename VarArray, typename Index, typename Result, bool undef_maps_zero = false>
struct ElementConstraintDynamic : public AbstractConstraint
{

  virtual string constraint_name()
  { return "watchelement"; }

  //  typedef BoolLessSumConstraintDynamic<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;

  // ReversibleInt count;
  VarArray var_array;
  Index indexvar;
  Result resultvar;

  virtual string full_output_name()
  {
    string undef_name = "";
    if(undef_maps_zero)
      undef_name = "_undefzero";

    vector<Mapper> v = indexvar.getMapperStack();
    if(!v.empty() && v.back() == Mapper(MAP_SHIFT, -1))
    {
      return ConOutput::print_con("watchelement_one"+undef_name, var_array, indexvar.popOneMapper(), resultvar);
    }
    else
    {
      return ConOutput::print_con("watchelement"+undef_name, var_array, indexvar, resultvar);
    }
  }

  DomainInt initial_result_dom_min;
  DomainInt initial_result_dom_max;

  vector<DomainInt> current_support;

  ElementConstraintDynamic(const VarArray& _var_array, const Index& _index, const Result& _result) :
    var_array(_var_array), indexvar(_index), resultvar(_result)
  {
    CheckNotBound(var_array, "watchelement", "element");
    CheckNotBoundSingle(indexvar, "watchelement", "element");
    CheckNotBoundSingle(resultvar, "watchelement", "element");
      initial_result_dom_min = std::min<DomainInt>(0,resultvar.getInitialMin());
      initial_result_dom_max = std::max<DomainInt>(0,resultvar.getInitialMax());
  }

  virtual SysInt dynamic_trigger_count()
  {
    SysInt count = var_array.size() * 2 +
    checked_cast<SysInt>(initial_result_dom_max - initial_result_dom_min + 1) * 2
    + 1
    + 1;
    if(undef_maps_zero)
      count++;
    current_support.resize(count / 2);           // is SysInt the right type?
    return count;
  }

  void find_new_support_for_result(SysInt j)
  {
    DynamicTrigger* dt = dynamic_trigger_start();

    DomainInt realj = j + initial_result_dom_min;

    if(!resultvar.inDomain(realj))
      return;

    if(undef_maps_zero && realj == 0)
    {
      if(indexvar.getMin() < 0)
      {
        moveTrigger(indexvar, dt+2*j, DomainRemoval, indexvar.getMin());
        releaseTrigger((dt+2*j+1));
        return;
      }

      if(indexvar.getMax() >= (SysInt)var_array.size())
      {
        moveTrigger(indexvar, dt+2*j, DomainRemoval, indexvar.getMax());
        releaseTrigger((dt+2*j+1));
        return;
      }
    }

    SysInt array_size = var_array.size();

    DomainInt indexvar_min = indexvar.getMin();
    DomainInt indexvar_max = indexvar.getMax();

    if(undef_maps_zero)
    {
      indexvar_min = std::max<DomainInt>(indexvar_min, 0);
      indexvar_max = std::min<DomainInt>(indexvar_max, (DomainInt)var_array.size() - 1);
    }

    D_ASSERT(indexvar_min >= 0);
    D_ASSERT(indexvar_max < (DomainInt)var_array.size());

    // support is value of index
    DomainInt oldsupport = max(current_support[j + array_size], indexvar_min);  // old support probably just removed
    DomainInt maxsupport = indexvar_max;

    DomainInt support = oldsupport;

    while(support <= maxsupport &&
          !(indexvar.inDomain(support) &&
            var_array[checked_cast<SysInt>(support)].inDomain(realj)))
      ++support;
    if(support > maxsupport)
    {
      support = indexvar_min;
      DomainInt max_check = min(oldsupport, maxsupport + 1);
      while(support < max_check &&
            !(indexvar.inDomain(support) &&
              var_array[checked_cast<SysInt>(support)].inDomain(realj)))
        ++support;
      if (support >= max_check)
      {
        resultvar.removeFromDomain(realj);
        return;
      }
    }
    moveTrigger(var_array[checked_cast<SysInt>(support)], dt + 2*j, DomainRemoval, realj);
    moveTrigger(indexvar, dt + 2*j + 1, DomainRemoval, support);
    current_support[j + array_size] = support;
  }

  void check_out_of_bounds_index()
  {
    if(!undef_maps_zero || !resultvar.inDomain(0))
    {
      indexvar.setMin(0);
      indexvar.setMax((DomainInt)var_array.size() - 1);
    }
  }

  void find_new_support_for_index(SysInt i)
  {
    if(!indexvar.inDomain(i))
      return;

    DomainInt resultvarmin = resultvar.getMin();
    DomainInt resultvarmax = resultvar.getMax();
    DynamicTrigger* dt = dynamic_trigger_start() +
                         checked_cast<SysInt>((initial_result_dom_max - initial_result_dom_min + 1) * 2);

    if(resultvarmin == resultvarmax)
    {
      if(!var_array[i].inDomain(resultvarmin))
        indexvar.removeFromDomain(i);
      else
      {
        moveTrigger(var_array[i], dt + 2*i, DomainRemoval, resultvarmin);
        moveTrigger(resultvar, dt + 2*i + 1, DomainRemoval, resultvarmin);
        current_support[i] = resultvarmin;
      }
      return;
    }


    // support is value of result
    DomainInt oldsupport = max(current_support[i], resultvarmin); // old support probably just removed
    DomainInt maxsupport = resultvarmax;
    DomainInt support = oldsupport;

    //SysInt support = initial_result_dom_min;
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

    moveTrigger(var_array[i], dt + 2*i, DomainRemoval, support);
    moveTrigger(resultvar, dt + 2*i + 1, DomainRemoval, support);
    current_support[i] = support;
  }

  void deal_with_assigned_index()
  {
    D_ASSERT(indexvar.isAssigned());
    SysInt indexval = checked_cast<SysInt>(indexvar.getAssignedValue());
    if(undef_maps_zero)
    {
      if(indexval < 0 || indexval >= (SysInt)var_array.size())
      {
        resultvar.propagateAssign(0);
        return;
      }
    }

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
    for(SysInt i=0; i<(SysInt)var_array.size(); i++) {
        if(var_array[i].isBound() && !var_array[i].isAssigned()) { // isassigned excludes constants.
            cerr << "Warning: watchelement is not designed to be used on bound variables and may cause crashes." << endl;
        }
    }
    if((indexvar.isBound() && !indexvar.isAssigned())
        || (resultvar.isBound() && !resultvar.isAssigned())) {
        cerr << "Warning: watchelement is not designed to be used on bound variables and may cause crashes." << endl;
    }

    SysInt array_size = var_array.size();
    DomainInt result_dom_size = initial_result_dom_max - initial_result_dom_min + 1;

    // Setup SupportLostForIndexValue(i,j)
    // Here we are supporting values in the index variable
    // So for each variable in the index variable, we want to ensure

    // Couple of quick sanity-propagations.
    // We define UNDEF = false ;)

    check_out_of_bounds_index();

    if(getState().isFailed()) return;

    for(SysInt i = 0; i < array_size; ++i)
    {
      current_support[i] = initial_result_dom_min-1;        // will be incremented if support sought
      if(indexvar.inDomain(i))
        find_new_support_for_index(i);
    }

    for(SysInt i = 0; i < result_dom_size; ++i)
    {
      current_support[i+array_size] = -1;   // will be incremented if support sought
      if(resultvar.inDomain(i + initial_result_dom_min))
        find_new_support_for_result(i);
    }

    if(indexvar.isAssigned())
      deal_with_assigned_index();

    DynamicTrigger* dt = dynamic_trigger_start();

    dt += var_array.size() * 2 +
      checked_cast<SysInt>((initial_result_dom_max - initial_result_dom_min + 1) * 2);

    // for(SysInt i = initial_result_dom_min; i <= initial_result_dom_max; ++i)
    // {
    // moveTrigger(resultvar, dt, DomainRemoval, i);
    // ++dt;
    // }
    moveTrigger(resultvar, dt, DomainChanged);  // Why is this always here-- why not place it when indexvar becomes assigned, lift it
    // whenever it triggers when indexvar is not assigned.
    ++dt;

    moveTrigger(indexvar, dt, Assigned);
    if(undef_maps_zero)
    {
      ++dt;
      if(resultvar.inDomain(0))
        moveTrigger(resultvar, dt, DomainRemoval, 0);
    }
  }


  virtual void propagate(DynamicTrigger* trig)
  {
    PROP_INFO_ADDONE(DynElement);
    DynamicTrigger* dt = dynamic_trigger_start();
    UnsignedSysInt pos = trig - dt;
    UnsignedSysInt array_size = var_array.size();
    UnsignedSysInt result_support_triggers =
      checked_cast<UnsignedSysInt>((initial_result_dom_max - initial_result_dom_min + 1) * 2);
    UnsignedSysInt index_support_triggers =  array_size * 2;
    // SysInt when_index_assigned_triggers = (initial_result_dom_max - initial_result_dom_min + 1);
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

    pos--;
    if(pos == 0)
    {
      deal_with_assigned_index();
      return;
    }

    if(undef_maps_zero)
    {
      // 0 has been lost from the domain
      D_ASSERT(pos == 1);
      D_ASSERT(!resultvar.inDomain(0));
      indexvar.setMin(0);
      indexvar.setMax((DomainInt)var_array.size() - 1);
      return;
    }

    D_FATAL_ERROR("Fatal error in watch-element");
  }

    virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
    {
      D_ASSERT(v_size == (SysInt)var_array.size() + 2);
      DomainInt resultvariable = v[v_size - 1];
      DomainInt indexvariable = v[v_size - 2];
      if(indexvariable < 0 || indexvariable >= (SysInt)v_size - 2)
      {
        if(undef_maps_zero)
          return resultvariable == 0;
        else
          return false;
      }

      return v[checked_cast<SysInt>(indexvariable)] == resultvariable;
    }

    virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size() + 2);
    for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
      vars.push_back(var_array[i]);
    vars.push_back(indexvar);
    vars.push_back(resultvar);
    return vars;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
    DomainInt array_start = max(DomainInt(0), indexvar.getMin());
    DomainInt array_end   = min(DomainInt(var_array.size()) - 1, indexvar.getMax());

    if(undef_maps_zero)
    {
      if(resultvar.inDomain(0))
      {
        if(indexvar.getMin() < 0)
        {
          assignment.push_back(make_pair(var_array.size(), indexvar.getMin()));
          assignment.push_back(make_pair(var_array.size() + 1, 0));
          return true;
        }
        if(indexvar.getMax() >= (SysInt)var_array.size())
        {
          assignment.push_back(make_pair(var_array.size(), indexvar.getMax()));
          assignment.push_back(make_pair(var_array.size() + 1, 0));
          return true;
        }
      }
    }

    for(SysInt i = checked_cast<SysInt>(array_start); i <= checked_cast<SysInt>(array_end); ++i)
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

      if(!undef_maps_zero) {
          // Constraint is satisfied if the index is out of range.
          vector<DomainInt> r; r.push_back(0); r.push_back((DomainInt)var_array.size()-1);
          AbstractConstraint* t4=(AbstractConstraint*) new WatchNotInRangeConstraint<Index>(indexvar, r);
          con.push_back(t4);
      }

      for(SysInt i=0; i<(SysInt)var_array.size(); i++)
      {
          vector<AbstractConstraint*> con2;
          WatchLiteralConstraint<Index>* t=new WatchLiteralConstraint<Index>(indexvar, i);
          con2.push_back((AbstractConstraint*) t);
          NeqConstraintBinary<AnyVarRef, Result>* t2=new NeqConstraintBinary<AnyVarRef, Result>(var_array[i], resultvar);
          con2.push_back((AbstractConstraint*) t2);

          Dynamic_AND* t3= new Dynamic_AND(con2);
          con.push_back((AbstractConstraint*) t3);
      }

      if(undef_maps_zero)
      {
        // or (i not in {0..size-1} /\ r!=0)
        vector<AbstractConstraint*> out_bounds;
        out_bounds.push_back(new WatchNotLiteralConstraint<Result>(resultvar, 0));
        out_bounds.push_back(new WatchNotInRangeConstraint<Index>(indexvar, make_vec<DomainInt>(0, (DomainInt)var_array.size() - 1)));
        con.push_back(new Dynamic_AND(out_bounds));
      }
      return new Dynamic_OR(con);
  }
};

template<typename Var1, typename Var2>
AbstractConstraint*
BuildCT_WATCHED_ELEMENT(const Var1& vararray, const Var2& v1, const Var1& v2, ConstraintBlob&)
{ 
  return new ElementConstraintDynamic<Var1, typename Var2::value_type, typename Var1::value_type>
              (vararray, v1[0], v2[0]);  
}

template<typename Var1, typename Var2, typename Var3>
AbstractConstraint*
BuildCT_WATCHED_ELEMENT(Var1 vararray, const Var2& v1, const Var3& v2, ConstraintBlob&)
{ 
  return new ElementConstraintDynamic<Var1, typename Var2::value_type, AnyVarRef>
              (vararray, v1[0], AnyVarRef(v2[0]));  
}

/* JSON
{ "type": "constraint",
  "name": "watchelement",
  "internal_name": "CT_WATCHED_ELEMENT",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

template<typename Var1, typename Var2, typename Var3>
AbstractConstraint*
BuildCT_WATCHED_ELEMENT_ONE(const Var1& vararray, const Var2& v1, const Var3& v2, ConstraintBlob& b)
{ 
  typedef typename ShiftType<typename Var2::value_type, compiletime_val<SysInt, -1> >::type ShiftVal;
  vector<ShiftVal> replace_v1;
  replace_v1.push_back(ShiftVarRef(v1[0], compiletime_val<SysInt, -1>()));
  return BuildCT_WATCHED_ELEMENT(vararray, replace_v1, v2, b);
}

/* JSON
{ "type": "constraint",
  "name": "watchelement_one",
  "internal_name": "CT_WATCHED_ELEMENT_ONE",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

template<typename Var1, typename Var2>
AbstractConstraint*
BuildCT_WATCHED_ELEMENT_UNDEFZERO(const Var1& vararray, const Var2& v1, const Var1& v2, ConstraintBlob&)
{ 
  return new ElementConstraintDynamic<Var1, typename Var2::value_type, typename Var1::value_type, true>
              (vararray, v1[0], v2[0]);  
}

template<typename Var1, typename Var2, typename Var3>
AbstractConstraint*
BuildCT_WATCHED_ELEMENT_UNDEFZERO(Var1 vararray, const Var2& v1, const Var3& v2, ConstraintBlob&)
{ 
  return new ElementConstraintDynamic<Var1, typename Var2::value_type, AnyVarRef, true>
              (vararray, v1[0], AnyVarRef(v2[0]));  
}

/* JSON
{ "type": "constraint",
  "name": "watchelement_undefzero",
  "internal_name": "CT_WATCHED_ELEMENT_UNDEFZERO",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

#endif
