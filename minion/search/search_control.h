/* Minion Constraint Solver
http://minion.sourceforge.net

For Licence Information see file LICENSE.txt 
*/

template<typename SearchAlgorithm, typename VarOrder, typename Vars, typename Propogator>
  void solve_select_search(StateObj* stateObj, VarOrder order_in, SearchAlgorithm& order, 
                           Vars& vars, CSPInstance& instance, Propogator prop)
{
  switch(order_in)
  {
    case ORDER_STATIC:
    case ORDER_SDF:
    case ORDER_LDF:
    case ORDER_ORIGINAL:
#ifdef WDEG
    case ORDER_WDEG:
#endif
    if(getOptions(stateObj).find_generators)
    {
      vector<AnyVarRef> perm = get_AnyVarRef_from_Var(stateObj, instance.permutation);
      Controller::group_solve_loop(stateObj, order, vars, perm, prop);
    }
    else
      Controller::solve_loop(stateObj, order, vars, prop);
    
    break;
    case ORDER_CONFLICT:
      Controller::conflict_solve_loop(stateObj, order, vars, prop);
    break;
  }
}

  template<typename VarValOrder, typename Propogator>
void solve(StateObj* stateObj, VarOrder order_in, VarValOrder& search_order, CSPInstance& instance, Propogator prop)
{
  typedef typename VarValOrder::first_type::value_type VarType;

  switch(order_in)
  {
    case ORDER_STATIC:
    {
      Controller::VariableOrder<VarType, Controller::SlowStaticBranch> 
        order(stateObj, search_order.first, search_order.second);

      try 
        { solve_select_search(stateObj, order_in, order, search_order.first, instance, prop); }
      catch(EndOfSearch)
        { }
    }
    break;
    case ORDER_SDF:
    {
      Controller::VariableOrder<VarType, Controller::SDFBranch> 
        order(stateObj, search_order.first, search_order.second);

      try 
        { solve_select_search(stateObj, order_in, order, search_order.first, instance, prop); }
      catch(EndOfSearch)
        { }
    }
    break;
    case ORDER_LDF:
    {
      Controller::VariableOrder<VarType, Controller::LDFBranch> 
        order(stateObj, search_order.first, search_order.second);

      try 
        { solve_select_search(stateObj, order_in, order, search_order.first, instance, prop); }
      catch(EndOfSearch)
        { }
    }
    break;

    case ORDER_ORIGINAL:
    {  
      Controller::VariableOrder<VarType, Controller::StaticBranch>
        order(stateObj, search_order.first, search_order.second);
      try
        { solve_select_search(stateObj, order_in, order, search_order.first, instance, prop); }
      catch(EndOfSearch)
        { }
    }
    break;
    case ORDER_CONFLICT:
    {
      Controller::VariableOrder<VarType, Controller::StaticBranch>
        order(stateObj, search_order.first, search_order.second);
      try
        { solve_select_search(stateObj, order_in, order, search_order.first, instance, prop); }
      catch(EndOfSearch)
        { }
    }
    break;
#ifdef WDEG
    case ORDER_WDEG:
    {
      Controller::VariableOrder<VarType, Controller::WdegBranch>
        order(stateObj, search_order.first, search_order.second);
      try
        { solve_select_search(stateObj, order_in, order, search_order.first, instance, prop); }
      catch(...)
        { }
    }
    break;
#endif
    default:
    FAIL_EXIT();
  } 
}

