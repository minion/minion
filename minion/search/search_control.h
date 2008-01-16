/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/


enum VarOrder
{
  ORDER_STATIC,
  ORDER_SDF,
  ORDER_LDF,
  ORDER_ORIGINAL
};

  template<typename VarValOrder>
  void solve(VarOrder order, VarValOrder& var_val_order)
  {
	switch(order)
	{
	  case ORDER_STATIC:
	  {
		Controller::VariableOrder<AnyVarRef, Controller::SlowStaticBranch> 
		order(var_val_order.first, var_val_order.second);
		
		try 
		{ Controller::solve_loop(order, var_val_order.first); }
		catch(...)
		{ }
	  }
		break;
	  case ORDER_SDF:
	  {
		   Controller::VariableOrder<AnyVarRef, Controller::SDFBranch> 
		   order(var_val_order.first, var_val_order.second);
		   
		   try 
		   { Controller::solve_loop(order, var_val_order.first); }
		   catch(...)
		   { }
	  }
	  break;
	  case ORDER_LDF:
	  {
		Controller::VariableOrder<AnyVarRef, Controller::LDFBranch> 
		order(var_val_order.first, var_val_order.second);
		
		try 
		{ Controller::solve_loop(order, var_val_order.first); }
		catch(...)
		{ }
	  }
		break;
		
	  case ORDER_ORIGINAL:
	  {  
		Controller::VariableOrder<AnyVarRef, Controller::StaticBranch>
		order(var_val_order.first, var_val_order.second);
		try
		{ Controller::solve_loop(order, var_val_order.first); }
		catch(...)
		{ }
	  }
		break;
	  default:
		FAIL_EXIT();
	} 
  }

