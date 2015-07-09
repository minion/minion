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

/** @help constraints;element_one Description
The constraint element one is identical to element, except that the
vector is indexed from 1 rather than from 0.
*/

/** @help constraints;element_one References
See

   help constraints element

for details of the element constraint which is almost identical to this
one.
*/

/** @help constraints;element Description
The constraint

   element(vec, i, e)

specifies that, in any solution, vec[i] = e and i is in the range
[0 .. |vec|-1].
*/

/** @help constraints;element Notes

Warning: This constraint is not confluent. Depending on the order the
propagators are called in Minion, the number of search nodes may vary when
using element. To avoid this problem, use watchelement instead. More details
below.

The level of propagation enforced by this constraint is not named, however it
works as follows. For constraint vec[i]=e:

- After i is assigned, ensures that min(vec[i]) = min(e) and
  max(vec[i]) = max(e).

- When e is assigned, removes idx from the domain of i whenever e is not an
  element of the domain of vec[idx].

- When m[idx] is assigned, removes idx from i when m[idx] is not in the domain
  of e.

This level of consistency is designed to avoid the propagator having to scan
through vec, except when e is assigned. It does a quantity of cheap propagation
and may work well in practise on certain problems.

Element is not confluent, which may cause the number of search nodes to vary
depending on the order in which constraints are listed in the input file, or
the order they are called in Minion. For example, the following input causes
Minion to search 41 nodes.

MINION 3
**VARIABLES**
DISCRETE x[5] {1..5}
**CONSTRAINTS**
element([x[0],x[1],x[2]], x[3], x[4])
alldiff([x])
**EOF**

However if the two constraints are swapped over, Minion explores 29 nodes.
As a rule of thumb, to get a lower node count, move element constraints
to the end of the list.
*/

/** @help constraints;element References
See the entry

   constraints watchelement

for details of an identical constraint that enforces generalised arc
consistency.
*/

#ifndef CONSTRAINT_ELEMENT_H_EIFESJKLNDFSNMDFS
#define CONSTRAINT_ELEMENT_H_EIFESJKLNDFSNMDFS


#include "constraint_equal.h"
#include "dynamic_new_or.h"
#include "dynamic_new_and.h"
#include "unary/dynamic_literal.h"
#include "unary/dynamic_notinrange.h"


template<typename VarArray, typename Index, typename Result, bool undef_maps_zero = false>
struct ElementConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "element"; }

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
      return ConOutput::print_con("element_one"+undef_name, var_array, indexvar.popOneMapper(), resultvar);
    }
    else
    {
      return ConOutput::print_con("element"+undef_name, var_array, indexvar, resultvar);
    }
  }

  ElementConstraint(const VarArray& _var_array, const Index& _indexvar, const Result& _resultvar) :
    var_array(_var_array), indexvar(_indexvar), resultvar(_resultvar)
  { }

  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    SysInt array_size = var_array.size();
    DomainInt loop_start = std::max(DomainInt(0), indexvar.getInitialMin());
    DomainInt loop_max = std::min(DomainInt(array_size) , indexvar.getInitialMax() + 1);
    for(DomainInt i = loop_start; i < loop_max; ++i)
      t.push_back(make_trigger(var_array[checked_cast<SysInt>(i)], Trigger(this, i), Assigned));

    t.push_back(make_trigger(indexvar, Trigger(this, -1), Assigned));
    t.push_back(make_trigger(resultvar, Trigger(this, -2), Assigned));
    return t;
  }

  virtual void propagateStatic(DomainInt prop_val, DomainDelta)
  {
    PROP_INFO_ADDONE(NonGACElement);
    if(indexvar.isAssigned())
    {
      SysInt index = checked_cast<SysInt>(indexvar.getAssignedValue());
      if(index < 0 || index >= (SysInt)var_array.size())
      {
          if(!undef_maps_zero) {
              getState().setFailed(true);
              return;
          }
          else {
              resultvar.propagateAssign(0);
              return;
          }
      }

      // Index is within bounds. Now both undefzero and standard ct behave the same.
      DomainInt val_min = max(resultvar.getMin(), var_array[index].getMin());
      DomainInt val_max = min(resultvar.getMax(), var_array[index].getMax());
      resultvar.setMin(val_min);
      var_array[index].setMin(val_min);
      resultvar.setMax(val_max);
      var_array[index].setMax(val_max);
    }
    else
    {
      if(prop_val>=0)
      {
        DomainInt assigned_val = var_array[checked_cast<SysInt>(prop_val)].getAssignedValue();
        if(indexvar.inDomain(prop_val) && !resultvar.inDomain(assigned_val))  //perhaps the check if prop_val is indomain of indexvar is not necessary.
        {
            if(indexvar.isBound())
            {
                if(prop_val==indexvar.getMax()) indexvar.setMax(prop_val-1);
                if(prop_val==indexvar.getMin()) indexvar.setMin(prop_val+1);
            }
            else
            {
                indexvar.removeFromDomain(prop_val);
            }
        }

      }
      else
      {
        D_ASSERT(prop_val == -2);
        DomainInt assigned_val = resultvar.getAssignedValue();
        SysInt array_size = var_array.size();
        for(SysInt i = 0; i < array_size; ++i)
        {
          if(indexvar.inDomain(i) && !var_array[i].inDomain(assigned_val)) // fixed here.
          {
              if(indexvar.isBound())
              {
                  if(i==indexvar.getMax()) indexvar.setMax(i-1);
                  if(i==indexvar.getMin()) indexvar.setMin(i+1);
              }
              else
              {
                  indexvar.removeFromDomain(i);
              }
          }
        }
      }
    }
  }

  virtual void full_propagate()
  {
    if(indexvar.isAssigned())
    {
      SysInt index = checked_cast<SysInt>(indexvar.getAssignedValue());
      if(!undef_maps_zero) {
          if(index < 0 || index >= (SysInt)var_array.size())
          {
            getState().setFailed(true);
            return;
          }
      }
      else {
          if(index < 0 || index >= (SysInt)var_array.size())
          {
            resultvar.setMin(0);
            resultvar.setMax(0);
            return;
          }
      }

      // Index assigned and within range.
      DomainInt val_min = max(resultvar.getMin(), var_array[index].getMin());
      DomainInt val_max = min(resultvar.getMax(), var_array[index].getMax());
      resultvar.setMin(val_min);
      var_array[index].setMin(val_min);
      resultvar.setMax(val_max);
      var_array[index].setMax(val_max);
    }

    SysInt array_size = var_array.size();
    // Constrain the index variable to have only indices in range, if undef_maps_zero is false.
    if(indexvar.getMin()<0 && !undef_maps_zero)
    {
        indexvar.setMin(0);
    }
    if(indexvar.getMax()>=array_size && !undef_maps_zero)
    {
        indexvar.setMax(array_size-1);
    }
    if(getState().isFailed()) return;

    // Should use the new iterators here. Check each value of resultvar to see
    // if it's in one of var_array.
    // Only done at root, so who cares that it takes a while?
    if(!resultvar.isBound())
    {
        for(DomainInt i=resultvar.getMin(); i<=resultvar.getMax(); i++)
        {
            if(i==0 && undef_maps_zero && (indexvar.getMin()<0 || indexvar.getMax()>=array_size) ) {
                // 0 is supported.
                continue;
            }
            if(resultvar.inDomain(i))
            {
                BOOL supported=false;
                for(DomainInt j=max(indexvar.getMin(), (DomainInt)0); j<=min(indexvar.getMax(), (DomainInt)array_size-1); j++)
                {
                    if(var_array[checked_cast<SysInt>(j)].inDomain(i))
                    {
                        supported=true;
                        break;
                    }
                }
                if(!supported)
                {
                    resultvar.removeFromDomain(i);
                }
            }
        }
    }
    else
    {// resultvar is a bound variable
        // iterate up from the minimum
        while(!getState().isFailed())
        {
            DomainInt i=resultvar.getMin();
            BOOL supported=false;
            for(DomainInt j=max(indexvar.getMin(),(DomainInt)0); j<=min(indexvar.getMax(), (DomainInt)array_size-1); j++)
            {
                if(var_array[checked_cast<SysInt>(j)].inDomain(i))
                {
                    supported=true;
                    break;
                }
            }
            if(i==0 && undef_maps_zero && (indexvar.getMin()<0 || indexvar.getMax()>=array_size) ) {
                // 0 is supported.
                supported=true;
            }
            if(!supported)
            {
                resultvar.setMin(i+1);
            }
            else
                break;
        }
        // now iterate down from the top.
        while(!getState().isFailed())
        {
            DomainInt i=resultvar.getMax();
            BOOL supported=false;
            for(DomainInt j=max(indexvar.getMin(),(DomainInt)0); j<=min(indexvar.getMax(), (DomainInt)array_size-1); j++)
            {
                if(var_array[checked_cast<SysInt>(j)].inDomain(i))
                {
                    supported=true;
                    break;
                }
            }
            if(i==0 && undef_maps_zero && (indexvar.getMin()<0 || indexvar.getMax()>=array_size) ) {
                // 0 is supported.
                supported=true;
            }
            if(!supported)
            {
                resultvar.setMax(i-1);
            }
            else
                break;
        }
    }

    if(getState().isFailed()) return;

    // Check values of index variable for support.
    for(DomainInt i = max(indexvar.getMin(),(DomainInt)0);i <= min(indexvar.getMax(), (DomainInt)array_size-1); i++)
    {
      if(indexvar.inDomain(i) && var_array[checked_cast<SysInt>(i)].isAssigned())
      {
        DomainInt assigned_val = var_array[checked_cast<SysInt>(i)].getAssignedValue();
        if(!resultvar.inDomain(assigned_val))
        {
            if(indexvar.isBound())
            {
                if(i==indexvar.getMax()) indexvar.setMax(i-1);
                if(i==indexvar.getMin()) indexvar.setMin(i+1);
            }
            else
            {

                indexvar.removeFromDomain(i);
            }
        }
      }
    }

    if(resultvar.isAssigned())
    {
      DomainInt assigned_val = resultvar.getAssignedValue();
      for(SysInt i = 0; i < array_size; ++i)
      {
        if(indexvar.inDomain(i) && !var_array[i].inDomain(assigned_val))
        {
            if(indexvar.isBound())
            {
                if(i==indexvar.getMax()) indexvar.setMax(i-1);
                if(i==indexvar.getMin()) indexvar.setMin(i+1);
            }
            else
            {
                indexvar.removeFromDomain(i);
            }
        }
      }
    }

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

      for(SysInt i=0; i < (SysInt)var_array.size(); i++)
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
BuildCT_ELEMENT(const Var1& vararray, const Var2& v1, const Var1& v2, ConstraintBlob&)
{ 
  return new ElementConstraint<Var1, typename Var2::value_type, typename Var1::value_type>
              (vararray, v1[0], v2[0]);  
}

template<typename Var1, typename Var2, typename Var3>
inline AbstractConstraint*
BuildCT_ELEMENT(Var1 vararray, const Var2& v1, const Var3& v2, ConstraintBlob&)
{ 
  return new ElementConstraint<Var1, typename Var2::value_type, AnyVarRef>
              (vararray, v1[0], AnyVarRef(v2[0]));  
}

/* JSON
{ "type": "constraint",
  "name": "element",
  "internal_name": "CT_ELEMENT",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

template<typename Var1, typename Var2, typename Var3>
AbstractConstraint*
BuildCT_ELEMENT_ONE(const Var1& vararray, const Var2& v1, const Var3& v2, ConstraintBlob& b)
{ 
  typedef typename ShiftType<typename Var2::value_type, compiletime_val<SysInt, -1> >::type ShiftVal;
  vector<ShiftVal> replace_v1;
  replace_v1.push_back(ShiftVarRef(v1[0], compiletime_val<SysInt, -1>()));
  return BuildCT_ELEMENT(vararray, replace_v1, v2, b);
}


/* JSON
{ "type": "constraint",
  "name": "element_one",
  "internal_name": "CT_ELEMENT_ONE",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

template<typename Var1, typename Var2>
inline AbstractConstraint*
BuildCT_ELEMENT_UNDEFZERO(const Var1& vararray, const Var2& v1, const Var1& v2, ConstraintBlob&)
{ 
  return new ElementConstraint<Var1, typename Var2::value_type, typename Var1::value_type, true>
              (vararray, v1[0], v2[0]);  
}

template<typename Var1, typename Var2, typename Var3>
AbstractConstraint*
BuildCT_ELEMENT_UNDEFZERO(Var1 vararray, const Var2& v1, const Var3& v2, ConstraintBlob&)
{ 
  return new ElementConstraint<Var1, typename Var2::value_type, AnyVarRef, true>
              (vararray, v1[0], AnyVarRef(v2[0]));  
}

/* JSON
{ "type": "constraint",
  "name": "element_undefzero",
  "internal_name": "CT_ELEMENT_UNDEFZERO",
  "args": [ "read_list", "read_var", "read_var" ]
}
*/

#endif
