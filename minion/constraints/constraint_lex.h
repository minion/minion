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



template<typename VarArray1, typename VarArray2, bool Less = false>
struct LexLeqConstraint : public Constraint
{
  virtual string constraint_name()
  { if(Less) return "LexLess"; else return "LexLeq"; }
  
  typedef LexLeqConstraint<VarArray2, VarArray1,!Less> NegConstraintType;
  typedef typename VarArray1::value_type ArrayVarRef1;
  typedef typename VarArray2::value_type ArrayVarRef2;
  
  ReversibleInt alpha;
  ReversibleInt beta;
  ReversibleInt F;
  
  VarArray1 x;
  VarArray2 y;
  
  LexLeqConstraint(const VarArray1& _x, const VarArray2& _y) :
    x(_x), y(_y)
  { D_ASSERT(x.size() == y.size()); }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_LEXCON,"Setting up Constraint");
    triggerCollection t;
    
    int x_size = x.size();
    for(int i=0; i < x_size; ++i)
    {
	  t.push_back(make_trigger(x[i], Trigger(this, i), LowerBound));
	  t.push_back(make_trigger(x[i], Trigger(this, i), UpperBound));
    }
    
    int y_size = y.size();
    for(int i=0; i < y_size; ++i)
    {
	  t.push_back(make_trigger(y[i], Trigger(this, i), LowerBound));
	  t.push_back(make_trigger(y[i], Trigger(this, i), UpperBound));
    }
    alpha.set(0);
    beta.set(100000);
    F.set(0);
    return t;
  }
  
  virtual Constraint* reverse_constraint()
  {
    return new LexLeqConstraint<VarArray2, VarArray1,!Less>(y,x);
  }
  
  void updateAlpha(int i) {
    int n = x.size();
    if(Less)
    {
      if(i == n || i == beta.get())
      {
		Controller::fail();
		return;
      }
      if (!x[i].isAssigned() || !y[i].isAssigned() ||
		  x[i].getAssignedValue() != y[i].getAssignedValue())  {
		alpha.set(i);
		propogate(i,0);
      }
      else updateAlpha(i+1);
    }
    else
    {
      while (i < n) {
		if (!x[i].isAssigned() || !y[i].isAssigned() ||
			x[i].getAssignedValue() != y[i].getAssignedValue())  {
		  alpha.set(i) ;
		  propogate(i,0) ;
		  return ;
		}
		i++ ;
      }
      F.set(true) ;
    }
    
  }
  
  ///////////////////////////////////////////////////////////////////////////////
  // updateBeta()
  void updateBeta(int i) {
    int a = alpha.get() ;
    while (i >= a) {
      if (x[i].getMin() < y[i].getMax()) {
		beta.set(i+1) ;
		if (!(x[i].getMax() < y[i].getMin())) propogate(i,0) ;
		return ;
      }
      i-- ;    
    }
    Controller::fail() ;
    
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
    D_INFO(0,DI_LEXCON,"Begin Propogation");
    if (F.get())
    {
      D_INFO(0,DI_LEXCON,"Already True");
      return ;
    }
    int a = alpha.get(), b = beta.get();
    
	//Not sure why we need this, but we seem to.
	if(b <= a)
	{
	  Controller::fail();
	  return;
	}
	
    if(Less)
    { if(i < a || i >=b) return; }
    else
    { if (i >= b) return ; }
    
    if (i == a && i+1 == b) {
      x[i].setMax(y[i].getMax()-1) ;
      y[i].setMin(x[i].getMin()+1) ;
      if (checkLex(i)) {
		F.set(true) ;
		return ;
      }
    }
    else if (i == a && i+1 < b) {
      x[i].setMax(y[i].getMax()) ;
      y[i].setMin(x[i].getMin()) ;
      if (checkLex(i)) {
		F.set(true) ;
		return ;
      }
      if (x[i].isAssigned() && y[i].isAssigned() && x[i].getAssignedValue() == y[i].getAssignedValue())
		updateAlpha(i+1) ;
    }
    else if (a < i && i < b) {
      if ((i == b-1 && x[i].getMin() == y[i].getMax()) || x[i].getMin() > y[i].getMax())
		updateBeta(i-1) ;
    }
  }
  
  virtual bool check_unsat(int unsat_val, DomainDelta)
  {
    D_INFO(0,DI_LEXCON,string("Checking Unsat")+to_string(Less));
    int a = alpha.get();
    if(unsat_val >= a)
    {
      int x_size = x.size();
      for(int i = a; i < x_size; ++i)
      {
		int xval = x[i].getMin();
		int yval = y[i].getMax();
		if(xval < yval) 
		{
		  alpha.set(i);
		  return false;
		}
		if(xval > yval)
		  return true;
      }
      if(Less)
		return true;
      else
      {
		alpha.set(x.size());
		return false;
      }
      
    }
    else
    {
      int xval = x[unsat_val].getMin();
      int yval = y[unsat_val].getMax();
      if (xval > yval)
		return true;
      else
		return false;
    }
    FAIL_EXIT();
  }
  
  virtual bool full_check_unsat()
  {
    alpha.set(0);
	return check_unsat(0, 0);
  }
  
  bool checkLex(int i) {
    if(Less)
    {
      return x[i].getMax() < y[i].getMin();
    }
    else
    {
      int n = x.size() ;
      if (i == n-1) return (x[i].getMax() <= y[i].getMin()) ;
      else return (x[i].getMax() < y[i].getMin());
    }
  }
  
  virtual void full_propogate()
  {
    D_INFO(1, DI_LEXCON, "Begin Full Propogate");
    int i, n = x.size() ;
    for (i = 0; i < n; i++) {
      if (!x[i].isAssigned()) break ;	
      if (!y[i].isAssigned()) break ;
      if (x[i].getAssignedValue() != y[i].getAssignedValue()) break ;
    }
    if (i < n) {
      alpha.set(i) ;
      if (checkLex(i)) {
		F.set(true) ;
		return ;
      }
      int betaBound = -1 ;
      for (; i < n; i++) {
		if (x[i].getMin() > y[i].getMax()) break ;
		if (x[i].getMin() == y[i].getMax()) {
		  if (betaBound == -1) betaBound = i ;	 
		}
		else betaBound = -1 ;
      }
      if(!Less)
      {
		if (i == n) beta.set(1000000) ;
		else if (betaBound == -1) beta.set(i) ;
		else beta.set(betaBound) ;
      }
      else
      {
		if (betaBound == -1)  beta.set(i);
		else if (betaBound == -1) beta.set(betaBound) ;
      }
      if (alpha.get() >= beta.get()) Controller::fail() ;
      propogate(alpha.get(),0) ;             //initial propagation, if necessary.
    }
    else 
    {
      if(Less)
		Controller::fail();
      else
		F.set(true);
    }
  }
  
  virtual bool check_assignment(vector<int> v)
  {
    D_ASSERT(v.size() == x.size() + y.size());
    size_t x_size = x.size();

	for(size_t i = 0;i < x_size; i++)
    {
      if(v[i] < v[i + x_size])
		return true;
      if(v[i] > v[i + x_size])
		return false;
    }
    if(Less)
      return false;
    else
      return true;
	  
  }
   
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> array_copy;
    for(unsigned int i=0;i<x.size();i++)
      array_copy.push_back(AnyVarRef(x[i]));
    
    for(unsigned int i=0;i<y.size();i++)
      array_copy.push_back(AnyVarRef(y[i]));
	return array_copy;
  }
};


template<typename VarArray1, typename VarArray2>
Constraint*
LexLeqCon(const VarArray1& x, const VarArray2& y)
{ return new LexLeqConstraint<VarArray1, VarArray2>(x,y); }

template<typename VarArray1, typename VarArray2>
Constraint*
LexLessCon(const VarArray1& x, const VarArray2& y)
{ return new LexLeqConstraint<VarArray1, VarArray2,true>(x,y); }

BUILD_CONSTRAINT2(CT_LEXLEQ, LexLeqCon)

BUILD_CONSTRAINT2(CT_LEXLESS, LexLessCon)
