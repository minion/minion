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

// This will become always true sooner or later.


/// (var1 = var2) = var3

#ifndef CONSTRAINT_EQUAL_H
#define CONSTRAINT_EQUAL_H

// New version written by PN with bound triggers.
// Also stronger in eq case: copies bounds across rather than just propagating on assignment. 
template<typename EqualVarRef1, typename EqualVarRef2, typename BoolVarRef>
struct ReifiedEqualConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "ReifiedEqual"; }
  
  EqualVarRef1 var1;
  EqualVarRef2 var2;
  BoolVarRef var3;
  ReifiedEqualConstraint(StateObj* _stateObj, EqualVarRef1 _var1, EqualVarRef2 _var2, BoolVarRef _var3) :
    AbstractConstraint(_stateObj), var1(_var1), var2(_var2), var3(_var3)
  {
      CHECK(var3.getInitialMin() >= 0, "Reification variables must have domain within {0,1}");
      CHECK(var3.getInitialMin() <= 1, "Reification variables must have domain within {0,1}");
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    t.push_back(make_trigger(var1, Trigger(this, 10), LowerBound));
    t.push_back(make_trigger(var1, Trigger(this, 11), UpperBound));
    t.push_back(make_trigger(var2, Trigger(this, 20), LowerBound));
    t.push_back(make_trigger(var2, Trigger(this, 21), UpperBound));
    t.push_back(make_trigger(var3, Trigger(this, 3), Assigned));
    return t;
  }
  
  // rewrite the following two functions.
  virtual void full_propagate()
  {
    if(var3.isAssigned())
    {
      if(var3.getAssignedValue() == 1)
        eqprop();
      else
      {
          if(var1.isAssigned())
          {
              diseqvar1assigned();
          }
          if(var2.isAssigned())
          {
              diseqvar2assigned();
          }
      }
    }
    else
    {   // r not assigned.
        check();
    }
  }
  
  virtual void propagate(int i, DomainDelta)
  {
    PROP_INFO_ADDONE(ReifyEqual);
    switch(i)
    {
      case 10:
          // var1 lower bound has moved
          if(var3.isAssigned())
          {
              if(var3.getAssignedValue()==1)
              {
                  var2.setMin(var1.getMin());
              }
              else
              { // not equal.     
                  diseq();
              }
          }
          else
          {
              check();
          }
        break;
        
    case 11:
        // var1 upper bound has moved.
          if(var3.isAssigned())
          {
              if(var3.getAssignedValue()==1)
              {
                  var2.setMax(var1.getMax());
              }
              else
              { // not equal.     
                  diseq();
              }
          }
          else
          {
              check();
          }
        break;        
        
      case 20:
          // var2 lower bound has moved.
          if(var3.isAssigned())
          {
              if(var3.getAssignedValue()==1)
              {
                  var1.setMin(var2.getMin());
              }
              else
              {
                  diseq();
              }
          }
          else
          {
              check();
          }
          break;
          
      case 21:
          // var2 upper bound has moved.
          if(var3.isAssigned())
          {
              if(var3.getAssignedValue()==1)
              {
                  var1.setMax(var2.getMax());
              }
              else
              {
                  diseq();
              }
          }
          else
          {
              check();
          }
          break;
          
      case 3:
          if(var3.getAssignedValue()==1)
          {
              eqprop();
          }
          else
          {
              diseq();
          }
          break;
    }
  }
  
  inline void eqprop()
  {
      var1.setMin(var2.getMin());
      var1.setMax(var2.getMax());
      var2.setMin(var1.getMin());
      var2.setMax(var1.getMax());
  }
  
  inline void check()
  {   // var1 or var2 has changed, so check
      if(var1.getMax()<var2.getMin() || var1.getMin()>var2.getMax())
      {   // not equal
          var3.propagateAssign(0);
      }
      if(var1.isAssigned() && var2.isAssigned()
          && var1.getAssignedValue()==var2.getAssignedValue())
      {   // equal
          var3.propagateAssign(1);
      }
  }
  
  inline void diseqvar1assigned()
  {
      DomainInt remove_val = var1.getAssignedValue();
      if(var2.isBound())
      {
        if(var2.getMin() == remove_val)
          var2.setMin(remove_val + 1);
        if(var2.getMax() == remove_val)
          var2.setMax(remove_val - 1);
      }
      else {
        var2.removeFromDomain(remove_val);
      }
  }
  
  inline void diseqvar2assigned()
  {
      DomainInt remove_val = var2.getAssignedValue();
      if(var1.isBound())
      {
        if(var1.getMin() == remove_val)
          var1.setMin(remove_val + 1);
        if(var1.getMax() == remove_val)
          var1.setMax(remove_val - 1);
      }
      else {
        var1.removeFromDomain(remove_val);
      }
  }
  
  inline void diseq()
  {
      if(var1.isAssigned())
      {
          diseqvar1assigned();
      }
      else if(var2.isAssigned())
      {
          diseqvar2assigned();
      }
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 3);
    D_ASSERT(v[2] == 0 || v[2] == 1);
    return (v[0] == v[1]) == v[2];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(3);
    vars.push_back(var1);
    vars.push_back(var2);
    vars.push_back(var3);
    return vars;
  }
};

// This is required for the following to have bound triggers
// when the variables are bound,
// and propagate confluently.
// Remove the define to go back to assignment triggers in all cases.
#define MAKECONFLUENT

template<typename VarRef1, typename VarRef2>
struct NeqConstraintBinary : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Neq(Binary)"; }
  
  VarRef1 var1;
  VarRef2 var2;
  
  
  NeqConstraintBinary(StateObj* _stateObj, const VarRef1& _var1, const VarRef2& _var2 ) :
    AbstractConstraint(_stateObj), var1(_var1), var2(_var2)
  { }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    #ifndef MAKECONFLUENT
    t.push_back(make_trigger(var1, Trigger(this, 1), Assigned));
    t.push_back(make_trigger(var2, Trigger(this, 2), Assigned));
    #else
    if(var1.isBound())
    {
        t.push_back(make_trigger(var1, Trigger(this, 3), UpperBound));
        t.push_back(make_trigger(var1, Trigger(this, 4), LowerBound));
    }
    else
    {
        t.push_back(make_trigger(var1, Trigger(this, 1), Assigned));
    }
    
    if(var2.isBound())
    {
        t.push_back(make_trigger(var2, Trigger(this, 5), UpperBound));
        t.push_back(make_trigger(var2, Trigger(this, 6), LowerBound));
    }
    else
    {
        t.push_back(make_trigger(var2, Trigger(this, 2), Assigned));
    }
    #endif
    return t;
  }
  
  virtual void propagate(int prop_val, DomainDelta)
  {
    PROP_INFO_ADDONE(BinaryNeq);
    if (prop_val == 1) {
      DomainInt remove_val = var1.getAssignedValue();
      if(var2.isBound())
      {
        if(var2.getMin() == remove_val)
          var2.setMin(remove_val + 1);
        if(var2.getMax() == remove_val)
          var2.setMax(remove_val - 1);
      }
      else {
        var2.removeFromDomain(remove_val);
      }
    }
    #ifdef MAKECONFLUENT
    else if(prop_val == 3)
    {   // ub moved var1
        if(var2.isAssigned() && var2.getAssignedValue()==var1.getMax())
            var1.setMax(var1.getMax()-1);
        if(var1.isAssigned())
        {
            var1assigned();
        }
    }
    else if(prop_val == 4)
    {   // lb moved var1
        if(var2.isAssigned() && var2.getAssignedValue()==var1.getMin())
            var1.setMin(var1.getMin()+1);
        if(var1.isAssigned())
        {
            var1assigned();
        }
    }
    else if(prop_val == 5)
    {   // ub moved var2
        if(var1.isAssigned() && var1.getAssignedValue()==var2.getMax())
            var2.setMax(var2.getMax()-1);
        if(var2.isAssigned())
        {
            var2assigned();
        }
    }
    else if(prop_val == 6)
    {   // lb moved var2
        if(var1.isAssigned() && var1.getAssignedValue()==var2.getMin())
            var2.setMin(var2.getMin()+1);
        if(var2.isAssigned())
        {
            var2assigned();
        }
    }
    #endif
    else
    {
      D_ASSERT(prop_val == 2);
      DomainInt remove_val = var2.getAssignedValue();
      if(var1.isBound())
      {
        if(var1.getMin() == remove_val)
          var1.setMin(remove_val + 1);
        if(var1.getMax() == remove_val)
          var1.setMax(remove_val - 1);
      }
      else {
        var1.removeFromDomain(remove_val);
      }
    }
  }
  
  inline void var1assigned()
  {
      DomainInt remove_val = var1.getAssignedValue();
      if(var2.isBound())
      {
        if(var2.getMin() == remove_val)
          var2.setMin(remove_val + 1);
        if(var2.getMax() == remove_val)
          var2.setMax(remove_val - 1);
      }
      else {
        var2.removeFromDomain(remove_val);
      }
  }
  
  inline void var2assigned()
  {
      DomainInt remove_val = var2.getAssignedValue();
      if(var1.isBound())
      {
        if(var1.getMin() == remove_val)
          var1.setMin(remove_val + 1);
        if(var1.getMax() == remove_val)
          var1.setMax(remove_val - 1);
      }
      else {
        var1.removeFromDomain(remove_val);
      }
  }
  
  virtual void full_propagate()
  {
    if(var1.isAssigned())
    { 
      DomainInt remove_val = var1.getAssignedValue();
      if(var2.isBound())
      {
        if(var2.getMin() == remove_val)
          var2.setMin(remove_val + 1);
        if(var2.getMax() == remove_val)
          var2.setMax(remove_val - 1);
      }
      else {
        var2.removeFromDomain(remove_val);
      }
    }
    if(var2.isAssigned())
    { 
      DomainInt remove_val = var2.getAssignedValue();
      if(var1.isBound())
      {
        if(var1.getMin() == remove_val)
          var1.setMin(remove_val + 1);
        if(var1.getMax() == remove_val)
          var1.setMax(remove_val - 1);
      }
      else {
        var1.removeFromDomain(remove_val);
      }
    }
  }
    
    virtual BOOL check_assignment(DomainInt* v, int v_size)
    {
      D_ASSERT(v_size == 2); 
      if(v[0]==v[1]) return false;
      return true;
    }
    
    virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    if(var1.getMin() != var2.getMax())
    {
      assignment.push_back(make_pair(0, var1.getMin()));
      assignment.push_back(make_pair(1, var2.getMax()));
      return true; 
    }
    
    if(var1.getMax() != var2.getMin())
    {
      assignment.push_back(make_pair(0, var1.getMax()));
      assignment.push_back(make_pair(1, var2.getMin()));
      return true;
    }
    
    D_ASSERT(var1.isAssigned() && var2.isAssigned());
    D_ASSERT(var1.getAssignedValue() == var2.getAssignedValue());
    return false;
  }
  
  
    virtual vector<AnyVarRef> get_vars()
    {
      vector<AnyVarRef> vars(2);
          vars[0] = var1;
          vars[1] = var2;
      return vars;
    }
};


template<typename EqualVarRef1, typename EqualVarRef2>
struct EqualConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Equal"; }
  
  EqualVarRef1 var1;
  EqualVarRef2 var2;
  EqualConstraint(StateObj* _stateObj, EqualVarRef1 _var1, EqualVarRef2 _var2) : AbstractConstraint(_stateObj),
    var1(_var1), var2(_var2)
  {}
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    t.push_back(make_trigger(var1, Trigger(this, 1), UpperBound));
    t.push_back(make_trigger(var1, Trigger(this, 2), LowerBound));
    t.push_back(make_trigger(var2, Trigger(this, 3), UpperBound));
    t.push_back(make_trigger(var2, Trigger(this, 4), LowerBound));
    return t;
  }
  
  virtual void full_propagate()
  {
    propagate(1,0);
    propagate(2,0);
    propagate(3,0);
    propagate(4,0);
  }
  
  virtual void propagate(int i, DomainDelta)
  {
    PROP_INFO_ADDONE(Equal);
    switch(i)
    {
      case 1:
        var2.setMax(var1.getMax());
        return;
      case 2:
        var2.setMin(var1.getMin());
        return;
      case 3:
        var1.setMax(var2.getMax());
        return;
      case 4:
        var1.setMin(var2.getMin());
        return;
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

template<typename EqualVarRef1, typename EqualVarRef2>
AbstractConstraint*
EqualCon(StateObj* stateObj, EqualVarRef1 var1, EqualVarRef2 var2)
{ return new EqualConstraint<EqualVarRef1, EqualVarRef2>(stateObj, var1,var2); }

template<typename EqualVarRef1, typename EqualVarRef2>
AbstractConstraint*
EqualMinusCon(StateObj* stateObj, EqualVarRef1 var1, EqualVarRef2 var2)
{ return new EqualConstraint<EqualVarRef1, VarNeg<EqualVarRef2> >(stateObj, var1,VarNegRef(var2)); }

template<typename Var1, typename Var2>
AbstractConstraint*
NeqConBinary(StateObj* stateObj, const Var1& var1, const Var2& var2)
{
  return new NeqConstraintBinary<Var1, Var2>(stateObj, var1, var2); 
}

#endif
