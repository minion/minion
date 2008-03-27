/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/




  template<typename VarValOrder, typename Propogator>
  void solve(StateObj* stateObj, VarOrder order_in, VarValOrder& search_order, Propogator prop)
  {
//    PropogateGAC prop = PropogateGAC();
    typedef typename VarValOrder::first_type::value_type VarType;
	switch(order_in)
	{
	  case ORDER_STATIC:
	  {
		Controller::VariableOrder<VarType, Controller::SlowStaticBranch> 
		order(stateObj, search_order.first, search_order.second);
		
		try 
		{ Controller::solve_loop(stateObj, order, search_order.first, prop); }
		catch(...)
		{ }
	  }
		break;
	  case ORDER_SDF:
	  {
		   Controller::VariableOrder<VarType, Controller::SDFBranch> 
		   order(stateObj, search_order.first, search_order.second);
		   
		   try 
		   { Controller::solve_loop(stateObj, order, search_order.first, prop); }
		   catch(...)
		   { }
	  }
	  break;
	  case ORDER_LDF:
	  {
		Controller::VariableOrder<VarType, Controller::LDFBranch> 
		order(stateObj, search_order.first, search_order.second);
		
		try 
		{ Controller::solve_loop(stateObj, order, search_order.first, prop); }
		catch(...)
		{ }
	  }
		break;
		
	  case ORDER_ORIGINAL:
	  {  
		Controller::VariableOrder<VarType, Controller::StaticBranch>
		order(stateObj, search_order.first, search_order.second);
		try
		{ Controller::solve_loop(stateObj, order, search_order.first, prop); }
		catch(...)
		{ }
	  }
        break;
      case ORDER_CONFLICT:
      {
        Controller::VariableOrder<VarType, Controller::StaticBranch>
		order(stateObj, search_order.first, search_order.second);
		try
		{ Controller::conflict_solve_loop(stateObj, order, search_order.first, prop); }
		catch(...)
		{ }
      }
        break;
#ifdef WDEG
      case ORDER_WDEG:
      {
        Controller::VariableOrder<VarType, Controller::WdegBranch>
		order(stateObj, search_order.first, search_order.second);
		try
		{ Controller::conflict_solve_loop(stateObj, order, search_order.first, prop); }
		catch(...)
		{ }
      }
        break;
#endif
	  default:
		FAIL_EXIT();
	} 
  }

