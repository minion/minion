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

/** @help constraints;eq Description
Constrain two variables to take equal values.
*/

/** @help constraints;eq Example
eq(x0,x1)
*/

/** @help constraints;eq Notes
Achieves bounds consistency.
*/

/** @help constraints;eq Reference
help constraints minuseq
*/

/** @help constraints;minuseq Description
Constraint

   minuseq(x,y)

ensures that x=-y.
*/

/** @help constraints;minuseq Reference
help constraints eq
*/


/** @help constraints;diseq Description
Constrain two variables to take different values.
*/

/** @help constraints;diseq Notes
Achieves arc consistency.
*/

/** @help constraints;diseq Example
diseq(v0,v1)
*/

#ifndef CONSTRAINT_GACEQUAL_H
#define CONSTRAINT_GACEQUAL_H

#include "constraint_equal.h"

template<typename EqualVarRef1, typename EqualVarRef2>
struct GACEqualConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "GACEqual"; }
  
  EqualVarRef1 var1;
  EqualVarRef2 var2;
  
  DynamicTrigger* dtvar2;   // The start of the block related to var2
  
  GACEqualConstraint(StateObj* _stateObj, EqualVarRef1 _var1, EqualVarRef2 _var2) : AbstractConstraint(_stateObj),
    var1(_var1), var2(_var2)
  { }
  
  int dynamic_trigger_count() {
      return var1.getInitialMax()-var1.getInitialMin()+var2.getInitialMax()-var2.getInitialMin()+2;
  }
  
  virtual void full_propagate() {
      dtvar2=dynamic_trigger_start() + var1.getInitialMax()-var1.getInitialMin() +1;
      
      for(int val=var1.getMin(); val<=var1.getMax(); val++) {
          if(!var2.inDomain(val)) {
              var1.removeFromDomain(val);
          }
      }
      for(int val=var2.getMin(); val<=var2.getMax(); val++) {
          if(!var1.inDomain(val)) {
              var2.removeFromDomain(val);
          }
      }
      
      
      for(int val=var1.getInitialMin(); val<=var1.getMax(); val++) {
          if(var1.inDomain(val)) {
              var1.addDynamicTrigger(dynamic_trigger_start()+val-var1.getInitialMin(), DomainRemoval, val );
          }
      }
      
      for(int val=var2.getInitialMin(); val<=var2.getMax(); val++) {
          if(var2.inDomain(val)) {
              var2.addDynamicTrigger(dtvar2+val-var2.getInitialMin(), DomainRemoval, val );
          }
      }
      
      
  }
  
  virtual void propagate(DynamicTrigger* dt)
  {
      if(dt<dtvar2) {
          int val=dt-dynamic_trigger_start()+var1.getInitialMin();
          var2.removeFromDomain(val);
      }
      else {
          int val=dt-dtvar2+var2.getInitialMin();
          var1.removeFromDomain(val);
      }
  }
  
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 2);
    return (v[0] == v[1]);
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(2);
    vars.push_back(var1);
    vars.push_back(var2);
    return vars;
  }
  
   virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
   {
     DomainInt min_val = max(var1.getMin(), var2.getMin());
     DomainInt max_val = min(var1.getMax(), var2.getMax());
     
     for(DomainInt i = min_val ; i <= max_val; ++i)
     {
       if(var1.inDomain(i) && var2.inDomain(i))
       {
         assignment.push_back(make_pair(0, i));
         assignment.push_back(make_pair(1, i));
         return true;
       } 
     }
     return false;
   }
   
   virtual AbstractConstraint* reverse_constraint()
   {
       return new NeqConstraintBinary<EqualVarRef1, EqualVarRef2>(stateObj, var1, var2);
   }
};

#endif
