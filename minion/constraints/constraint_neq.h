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


/** @help constraints;alldiff Description
Forces the input vector of variables to take distinct values.
*/

/** @help constraints;alldiff Example
Suppose the input file had the following vector of variables defined:

DISCRETE myVec[9] {1..9}

To ensure that each variable takes a different value include the
following constraint:

alldiff(myVec)
*/

/** @help constraints;alldiff Notes
Enforces the same level of consistency as a clique of not equals
constraints.
*/

/** @help constraints;alldiff References
See

   help constraints gacalldiff

for the same constraint that enforces GAC.
*/

#ifndef CONSTRAINT_NEQ_H
#define CONSTRAINT_NEQ_H

#include "constraint_checkassign.h"

template<typename VarArray>
struct NeqConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "alldiff"; }

  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;

  VarArray var_array;

  CONSTRAINT_ARG_LIST1(var_array);

  NeqConstraint(const VarArray& _var_array) : 
    var_array(_var_array)
  { }

  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    SysInt array_size = var_array.size();
    for(SysInt i = 0; i < array_size; ++i)
      t.push_back(make_trigger(var_array[i], Trigger(this, i), Assigned));
    return t;
  }

  virtual AbstractConstraint* reverse_constraint()
  { return forward_check_negation(this); }

  virtual void propagate(DomainInt prop_val_in, DomainDelta)
  {
    const SysInt prop_val = checked_cast<SysInt>(prop_val_in);
    PROP_INFO_ADDONE(ArrayNeq);
    DomainInt remove_val = var_array[prop_val].getAssignedValue();
    SysInt array_size = var_array.size();
    for(SysInt i = 0; i < array_size; ++i)
    {
      if(i != prop_val)
      {
        if(var_array[i].isBound())
        {
          if(var_array[i].getMin() == remove_val)
            var_array[i].setMin(remove_val + 1);
          if(var_array[i].getMax() == remove_val)
            var_array[i].setMax(remove_val - 1);
        }
        else {
          var_array[i].removeFromDomain(remove_val);
        }
      }
    }

  }

  virtual void full_propagate()
  {
    SysInt array_size = var_array.size();
    for(SysInt i = 0; i < array_size; ++i)
      if(var_array[i].isAssigned())
      {
        DomainInt remove_val = var_array[i].getAssignedValue();
        for(SysInt j = 0; j < array_size;++j)
        {
          if(i != j)
          {
            if(var_array[j].isBound())
            {
              if(var_array[j].getMin() == remove_val)
                var_array[j].setMin(remove_val + 1);
              if(var_array[j].getMax() == remove_val)
                var_array[j].setMax(remove_val - 1);
            }
            else {
              var_array[j].removeFromDomain(remove_val);
            }
          }
        }
      }
  }

    virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
    {
      D_ASSERT(v_size == (SysInt)var_array.size());
      SysInt array_size = checked_cast<SysInt>(v_size);
      for(SysInt i=0;i<array_size;i++)
        for( SysInt j=i+1;j<array_size;j++)
          if(v[i]==v[j]) return false;
      return true;
    }

    virtual vector<AnyVarRef> get_vars()
    {
      vector<AnyVarRef> vars;
      vars.reserve(var_array.size());
      for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
        vars.push_back(var_array[i]);
      return vars;
    }


     // Getting a satisfying assignment here is too hard, we don't want to have to
     // build a matching.
     virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
   {
     MAKE_STACK_BOX(c, DomainInt, var_array.size());

     for(UnsignedSysInt i = 0; i < var_array.size(); ++i)
     {
       if(!var_array[i].isAssigned())
       {
         assignment.push_back(make_pair(i, var_array[i].getMin()));
         assignment.push_back(make_pair(i, var_array[i].getMax()));
         return true;
       }
       else
         c.push_back(var_array[i].getAssignedValue());
     }

    if(check_assignment(c.begin(), c.size()))
    {  // Put the complete assignment in the box.
      for(SysInt i = 0; i < (SysInt)var_array.size(); ++i)
        assignment.push_back(make_pair(i, c[i]));
      return true;
    }
    return false;
   }


  };
  
  template<typename VarArray>
  AbstractConstraint*
  BuildCT_ALLDIFF(const VarArray& var_array, ConstraintBlob&)
  { return new NeqConstraint<VarArray>(var_array); }
  
  /* JSON
  { "type": "constraint",
    "name": "alldiff",
    "internal_name": "CT_ALLDIFF",
    "args": [ "read_list" ]
  }
  */
  
#endif
