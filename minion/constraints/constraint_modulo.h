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

/** @help constraints;modulo Description
The constraint
 
   modulo(x,y,z)

ensures that x%y=z i.e. z is the remainder of dividing x by y.
*/

/** @help constraints;modulo Notes
This constraint is only available for positive domains x, y and z.
*/

/** @help constraints;modulo References
help constraints div
*/

#ifndef CONSTRAINT_MODULO_H
#define CONSTRAINT_MODULO_H

template<typename VarRef1, typename VarRef2, typename VarRef3>
struct NotModConstraint : public AbstractConstraint
{
    virtual string constraint_name()
  { return "NotModulo"; }
  
  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;

  NotModConstraint(StateObj* _stateObj, VarRef1 _var1, VarRef2 _var2, VarRef3 _var3) :
    AbstractConstraint(_stateObj), var1(_var1), var2(_var2), var3(_var3)
  {
  
      if(var1.getInitialMin() < 0 || var2.getInitialMin() < 1 ||
         var3.getInitialMin() < 0)
      { 
      FAIL_EXIT("The negated 'modulo' constraint only supports nonnegative numbers, and positive bases, at present.");
      }
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    if(var1.isBound())
    {
        t.push_back(make_trigger(var1, Trigger(this, 1), LowerBound));
        t.push_back(make_trigger(var1, Trigger(this, 1), UpperBound));
    }
    else
    {
        t.push_back(make_trigger(var1, Trigger(this, 1), Assigned));
    }
    
    if(var2.isBound())
    {
        t.push_back(make_trigger(var2, Trigger(this, 1), LowerBound));
        t.push_back(make_trigger(var2, Trigger(this, 1), UpperBound));
    }
    else
    {
        t.push_back(make_trigger(var2, Trigger(this, 1), Assigned));
    }
    
    if(var3.isBound())
    {
        t.push_back(make_trigger(var3, Trigger(this, 1), LowerBound));
        t.push_back(make_trigger(var3, Trigger(this, 1), UpperBound));
    }
    else
    {
        t.push_back(make_trigger(var3, Trigger(this, 1), Assigned));
    }
    return t;
  }
  
  PROPAGATE_FUNCTION(int flag, DomainDelta)
  {
    PROP_INFO_ADDONE(Mod);
    // propagate var1 % var2 != var3 by forward checking
    if(var1.isAssigned() && var2.isAssigned())
    {
        if(!var3.isBound())
        {
            var3.removeFromDomain(var1.getAssignedValue() % var2.getAssignedValue());
        }
        else
        {
            DomainInt temp=var1.getAssignedValue() % var2.getAssignedValue();
            if(temp==var3.getMin())
                var3.setMin(temp+1);
            else if(temp==var3.getMax())
                var3.setMax(temp-1);
        }
    }
    else if(var1.isAssigned() && var3.isAssigned())
    {
        if(!var2.isBound())
        {
            for(DomainInt i=var2.getMin(); i<=var2.getMax(); i++)
            {
                if(var1.getAssignedValue() % i == var3.getAssignedValue())
                {
                    var2.removeFromDomain(i);
                }
            }
        }
        else
        {
            if(var1.getAssignedValue() % var2.getMin() == var3.getAssignedValue())
            {
                var2.setMin(var2.getMin()+1);
            }
            else if(var1.getAssignedValue() % var2.getMax() == var3.getAssignedValue())
            {
                var2.setMax(var2.getMax()-1);
            }
        }
    }
    else if(var2.isAssigned() && var3.isAssigned())
    {
        if(!var1.isBound())
        {
            for(DomainInt i=var1.getMin(); i<=var1.getMax(); i++)
            {
                if(i % var2.getAssignedValue() == var3.getAssignedValue())
                {
                    var1.removeFromDomain(i);
                    
                    // skip a few iterations to the next interesting value.
                    i=i+var2.getAssignedValue()-1;
                }
            }
        }
        else
        {
            if(var1.getMin() % var2.getAssignedValue() == var3.getAssignedValue())
            {
                var1.setMin(var1.getMin()+1);
            }
            else if(var1.getMax() % var2.getAssignedValue() == var3.getAssignedValue())
            {
                var1.setMax(var1.getMax()-1);
            }
        }
    }
    
  }
  
  virtual void full_propagate()
  { 
    propagate(1,0);
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 3);
    return v[0] % v[1] != v[2];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> v;
    v.push_back(var1);
    v.push_back(var2);
    v.push_back(var3);
    return v;
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {  
    for(DomainInt v1 = var1.getMin(); v1 <= var1.getMax(); ++v1)
    {
      if(var1.inDomain(v1))
      {
        for(DomainInt v2 = var2.getMin(); v2 <= var2.getMax(); ++v2)
        {
          if(var2.inDomain(v2))
          {
              if(var3.getMin()!= v1 % v2)
              {
                  assignment.push_back(make_pair(0, v1));
                  assignment.push_back(make_pair(1, v2));
                  assignment.push_back(make_pair(2, var3.getMin()));
                  return true;
              }
              if(var3.getMax()!= v1 % v2)
              {
                  assignment.push_back(make_pair(0, v1));
                  assignment.push_back(make_pair(1, v2));
                  assignment.push_back(make_pair(2, var3.getMax()));
                  return true;
              }
          }
        }
      }
    }
    return false;
  }
};


/// var1 % var2 = var3
template<typename VarRef1, typename VarRef2, typename VarRef3>
struct ModConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Modulo"; }
  
  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;

  ModConstraint(StateObj* _stateObj, VarRef1 _var1, VarRef2 _var2, VarRef3 _var3) :
    AbstractConstraint(_stateObj), var1(_var1), var2(_var2), var3(_var3)
  {
  
      if(var1.getInitialMin() < 0 || var2.getInitialMin() < 1 ||
         var3.getInitialMin() < 0)
      { 
      FAIL_EXIT("The 'modulo' constraint only supports nonnegative numbers, and positive bases, at present.");
      }
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    t.push_back(make_trigger(var1, Trigger(this, -1), LowerBound));
    t.push_back(make_trigger(var2, Trigger(this, -2), LowerBound));
    t.push_back(make_trigger(var3, Trigger(this, -3), LowerBound));
    t.push_back(make_trigger(var1, Trigger(this, 1), UpperBound));
    t.push_back(make_trigger(var2, Trigger(this, 2), UpperBound));
    t.push_back(make_trigger(var3, Trigger(this, 3), UpperBound));
    return t;
  }
  
  PROPAGATE_FUNCTION(int flag, DomainDelta)
  {
    PROP_INFO_ADDONE(Mod);
    // aiming at bounds(D)-consistency. 
    // Not achieved because we don't trigger on the supporting values, only bounds.
    
    // var1 upperbound
    DomainInt var1val=var1.getMax();
    while(!testsupport_var1(var1val))
    {
        // While no support for upperbound, reduce upperbound
        var1.setMax(var1val-1);
        var1val=var1.getMax();
        if(getState(stateObj).isFailed()) return;
    }
    
    var1val=var1.getMin();
    while(!testsupport_var1(var1val))
    {
        var1.setMin(var1val+1);
        var1val=var1.getMin();
        if(getState(stateObj).isFailed()) return;
    }
    
    DomainInt var2val=var2.getMax();
    while(!testsupport_var2(var2val))
    {
        // While no support for upperbound, reduce upperbound
        var2.setMax(var2val-1);  // Is this the right function for pruning the upperbound?
        var2val=var2.getMax();
        if(getState(stateObj).isFailed()) return;
    }
    
    var2val=var2.getMin();
    while(!testsupport_var2(var2val))
    {
        var2.setMin(var2val+1);
        var2val=var2.getMin();
        if(getState(stateObj).isFailed()) return;
    }
    
    DomainInt var3val=var3.getMax();
    while(!testsupport_var3(var3val))
    {
        // While no support for upperbound, reduce upperbound
        var3.setMax(var3val-1);  // Is this the right function for pruning the upperbound?
        var3val=var3.getMax();
        if(getState(stateObj).isFailed()) return;
    }
    
    var3val=var3.getMin();
    while(!testsupport_var3(var3val))
    {
        var3.setMin(var3val+1);
        var3val=var3.getMin();
        if(getState(stateObj).isFailed()) return;
    }
    
  }
  
  // next step is to modify the following three functions to store the supporting tuple,
  // then watch the literals in that tuple. 
  
  inline BOOL testsupport_var1(DomainInt var1val)
  {
    // If this returns true var1val would be supported by
    // var2=var2.getMax , var3=var1val
    
    if(var2.getMax()>var1val && var3.inDomain(var1val))
    {
        return true;
    }
    for(DomainInt var2val=var2.getMin(); var2val<=var2.getMax() && var2val<=var1val; ++var2val)
    {
        if(var2.inDomain(var2val) && var3.inDomain(var1val % var2val))
        {
            return true;
        }
    }
    return false;
  }
  
  inline BOOL testsupport_var2(DomainInt var2val)
  {
      // var1 % var2 = var3
      for(DomainInt var1val=var1.getMin(); var1val<=var1.getMax(); ++var1val)
      {
          if(var1.inDomain(var1val) && var3.inDomain(var1val % var2val))
          {
              return true;
          }
      }
      return false;
  }
  
  inline BOOL testsupport_var3(DomainInt var3val)
  {
      // Iterate through var2 then
      // through the values of var1 which are congruent with var3val.
      DomainInt var2val=var2.getMin();
      
      if(var2val<=var3val) var2val=var3val+1;  // var3val<var2val always.
        for(; var2val<=var2.getMax(); ++var2val)
        {
            if(var2.inDomain(var2val))
            {
                DomainInt var1val=var1.getMin();
                // need to jump to the next value which is congruent with var3 (mod var2)
                DomainInt mod=var1val%var2val;
                if(mod<var3val)
                {
                    var1val+=(var3val-mod);
                    //if((var1val%var2val) != var3val) {cerr << "What on earth?1" << endl;}
                }
                if(mod>var3val)
                {
                    var1val+=(var3val-mod)+var2val;
                    //if((var1val%var2val) != var3val) {cerr << "What on earth?2" << endl;}
                }
                //if((var1val%var2val) != var3val) {cerr << var1val <<","<<var2val<<","<<var3val << endl;}
                for(; var1val<=var1.getMax(); var1val+=var2val) 
                    // Skips values which are not congruent.
                {
                    if(var1.inDomain(var1val) && (var1val % var2val)==var3val)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
  }
  
  virtual void full_propagate()
  { 
    propagate(1,0); 
    /*propagate(2,0);
    propagate(3,0);
    propagate(-1,0);
    propagate(-2,0);
    propagate(-3,0);*/
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 3);
    return v[0] % v[1] == v[2];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> v;
    v.push_back(var1);
    v.push_back(var2);
    v.push_back(var3);
    return v;
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {  
    for(DomainInt v1 = var1.getMin(); v1 <= var1.getMax(); ++v1)
    {
      if(var1.inDomain(v1))
      {
        for(DomainInt v2 = var2.getMin(); v2 <= var2.getMax(); ++v2)
        {
          if(var2.inDomain(v2) && var3.inDomain(v1 % v2))
          {
            assignment.push_back(make_pair(0, v1));
            assignment.push_back(make_pair(1, v2));
            assignment.push_back(make_pair(2, v1 % v2));
            return true;
          }
        }
      }
    }
    return false;
  }
  
  // Function to make it reifiable in the most minimal way.
    virtual AbstractConstraint* reverse_constraint()
    {
        return new NotModConstraint<VarRef1, VarRef2, VarRef3>(stateObj, var1, var2, var3);
    }
};

template<typename V1, typename V2>
inline AbstractConstraint*
BuildCT_MODULO(StateObj* stateObj, const V1& vars, const V2& var2, ConstraintBlob&)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return new ModConstraint<typename V1::value_type, typename V1::value_type,
                           typename V2::value_type>(stateObj, vars[0], vars[1], var2[0]);
}

#endif
