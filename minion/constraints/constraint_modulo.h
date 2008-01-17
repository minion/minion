/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: constraint_product.h 472 2006-11-17 17:04:36Z azumanga $
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

/// var1 % var2 = var3
template<typename VarRef1, typename VarRef2, typename VarRef3>
struct ModConstraint : public Constraint
{
  virtual string constraint_name()
  { return "XXX"; }
  
  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;

  ModConstraint(VarRef1 _var1, VarRef2 _var2, VarRef3 _var3) :
	var1(_var1), var2(_var2), var3(_var3)
  {
  
	  if(var1.getInitialMin() < 0 || var2.getInitialMin() < 1 ||
		 var3.getInitialMin() < 0)
	  { 
		cerr << "The 'modulo' constraint only supports nonnegative numbers, and positive bases, at present.";
		exit(1);
	  }
  }
  
  virtual triggerCollection setup_internal()
  {
	D_INFO(2,DI_ANDCON,"Setting up Constraint");
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
        if(state->isFailed()) return;  // Does not work with setjmp!
    }
    
    var1val=var1.getMin();
    while(!testsupport_var1(var1val))
    {
        var1.setMin(var1val+1);
        var1val=var1.getMin();
        if(state->isFailed()) return;
    }
    
    DomainInt var2val=var2.getMax();
    while(!testsupport_var2(var2val))
    {
        // While no support for upperbound, reduce upperbound
        var2.setMax(var2val-1);  // Is this the right function for pruning the upperbound?
        var2val=var2.getMax();
        if(state->isFailed()) return;  // Does not work with setjmp!
    }
    
    var2val=var2.getMin();
    while(!testsupport_var2(var2val))
    {
        var2.setMin(var2val+1);
        var2val=var2.getMin();
        if(state->isFailed()) return;
    }
    
    DomainInt var3val=var3.getMax();
    while(!testsupport_var3(var3val))
    {
        // While no support for upperbound, reduce upperbound
        var3.setMax(var3val-1);  // Is this the right function for pruning the upperbound?
        var3val=var3.getMax();
        if(state->isFailed()) return;  // Does not work with setjmp!
    }
    
    var3val=var3.getMin();
    while(!testsupport_var3(var3val))
    {
        var3.setMin(var3val+1);
        var3val=var3.getMin();
        if(state->isFailed()) return;
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
                    if((var1val%var2val) != var3val) {cerr << "What on earth?1" << endl;}
                }
                if(mod>var3val)
                {
                    var1val+=(var3val-mod)+var2val;
                    if((var1val%var2val) != var3val) {cerr << "What on earth?2" << endl;}
                }
                if((var1val%var2val) != var3val) {cerr << var1val <<","<<var2val<<","<<var3val << endl;}
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
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
	D_ASSERT(v.size() == 3);
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
};

template<typename V1, typename V2>
inline Constraint*
ModuloCon(const V1& vars, const V2& var2)
{
  D_ASSERT(vars.size() == 2);
  D_ASSERT(var2.size() == 1);
  return new ModConstraint<typename V1::value_type, typename V1::value_type,
						   typename V2::value_type>(vars[0], vars[1], var2[0]);
}


BUILD_CONSTRAINT2(CT_MODULO, ModuloCon);
