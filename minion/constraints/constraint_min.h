/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/




template<typename VarArray, typename MinVarRef>
struct MinConstraint : public Constraint
{
  virtual string constraint_name()
  { return "Min"; }
  
  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type ArrayVarRef;
  
  VarArray var_array;
  MinVarRef min_var;
  
  MinConstraint(const VarArray& _var_array, const MinVarRef& _min_var) :
	var_array(_var_array), min_var(_min_var)
  { }
  
  virtual triggerCollection setup_internal()
  {
	D_INFO(2,DI_SUMCON,"Setting up Constraint");
	triggerCollection t;
	
	for(unsigned i = 0; i < var_array.size(); ++i)
	{ // Have to add 1 else the 0th element will be lost.
	  t.push_back(make_trigger(var_array[i], Trigger(this, i + 1), LowerBound));
	  t.push_back(make_trigger(var_array[i], Trigger(this, -(i + 1)), UpperBound));
	}
	t.push_back(make_trigger(min_var, Trigger(this, var_array.size() + 1 ),LowerBound));
	t.push_back(make_trigger(min_var, Trigger(this, -(int)var_array.size() + 1 ),UpperBound));
	
	return t;
  }
  
  //  virtual Constraint* reverse_constraint()
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
   {
	PROP_INFO_ADDONE(Min);
	if(prop_val > 0)
	{// Lower Bound Changed
	  
	  //Had to add 1 to fix "0th array" problem.
	  --prop_val;
	  
	  if(prop_val == (int)(var_array.size()))  
	  {
		DomainInt new_min = min_var.getMin();
		typename VarArray::iterator end = var_array.end();
		for(typename VarArray::iterator it = var_array.begin(); it < end; ++it)
		  (*it).setMin(new_min);
	  }
	  else
	  {
	    typename VarArray::iterator it = var_array.begin();
		typename VarArray::iterator end = var_array.end();
		DomainInt min = it->getMin();
		++it;
		for(; it < end; ++it)
		{
		  DomainInt it_min = it->getMin();
		  if(it_min < min)
			min = it_min;
		}
		min_var.setMin(min);
	  }
	}
	else
	{// Upper Bound Changed
	  // See above for reason behind "-1".
	  prop_val = -prop_val - 1;
	  if(prop_val == (int)(var_array.size()))
	  {
		typename VarArray::iterator it = var_array.begin();
		DomainInt minvar_max = min_var.getMax();
		while(it != var_array.end() && (*it).getMin() > minvar_max)
		  ++it;
		if(it == var_array.end())
		{
		  Controller::fail();
		  return;
		}
		
		// Possibly this variable is the only one that can be the minimum
		typename VarArray::iterator it_copy(it);
		++it;
		while(it != var_array.end() && (*it).getMin() > minvar_max)
		  ++it;
		if(it != var_array.end())
		{ // No, another variable can be the minimum
		  return;
		}
		
		it_copy->setMax(minvar_max);
	  }
	  else
	  {
		min_var.setMax(var_array[prop_val].getMax());
	  }
	}
	
  }
  
  //  virtual BOOL check_unsat(int i, DomainDelta)
  //  {
  
  
  virtual void full_propogate()
  {
	int array_size = var_array.size();
	for(int i = 1;i <= array_size + 1; ++i)
	{
	  propogate(i,0);
	  propogate(-i,0);
	}
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
	D_ASSERT(v.size() == var_array.size() + 1);
	DomainInt min_val = big_constant;
	int array_size = v.size();
	for(int i=0;i < array_size - 1;i++)
	  min_val = min(min_val, v[i]);
	return min_val == v.back();
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
	vars.reserve(var_array.size() + 1);
	for(unsigned i = 0; i < var_array.size(); ++i)
	  vars.push_back(AnyVarRef(var_array[i]));
	vars.push_back(AnyVarRef(min_var));
	return vars;
  }
};

template<typename VarArray, typename VarRef>
Constraint*
MinCon(const VarArray& _var_array, const light_vector<VarRef>& _var_ref)
{ return (new MinConstraint<VarArray,VarRef>(_var_array, _var_ref[0])); }

template<typename VarArray, typename VarRef>
Constraint*
MaxCon(const VarArray& _var_array, const light_vector<VarRef>& _var_ref)
{ return (new MinConstraint<typename NegType<VarArray>::type, typename NegType<VarRef>::type>(VarNegRef(_var_array),
																					          VarNegRef(_var_ref[0]))); 
}


BUILD_CONSTRAINT2(CT_MAX, MaxCon)

BUILD_CONSTRAINT2(CT_MIN, MinCon)

