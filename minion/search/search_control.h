/* Minion Constraint Solver
http://minion.sourceforge.net

For Licence Information see file LICENSE.txt 
*/
#include <boost/bind.hpp>
using boost::bind;

#include "../system/system.h"
#include "common_search.h"
#include "standard_search.h"

template<typename SearchAlgorithm, typename Vars, typename Propogator>
  function<void (void)> solve_select_search(StateObj* stateObj, const function<void (void)>& next_search, SearchOrder order_in, 
                                                 SearchAlgorithm& order, Vars& vars, CSPInstance& instance, Propogator prop)
{
  switch(order_in.order)
  {
    case ORDER_STATIC:
    case ORDER_SDF:
    case ORDER_SRF:
    case ORDER_LDF:
    case ORDER_ORIGINAL:
    case ORDER_WDEG:
    case ORDER_DOMOVERWDEG:
    if(getOptions(stateObj).find_generators)
    {
      vector<AnyVarRef> perm = get_AnyVarRef_from_Var(stateObj, instance.permutation);
      return bind(Controller::group_solve_loop<SearchAlgorithm, Vars, vector<AnyVarRef>, Propogator>, stateObj, next_search, order, vars, perm, prop);
    }
    else
      return bind(Controller::solve_loop<SearchAlgorithm, Vars, Propogator>, stateObj, next_search, order, vars, prop, order_in.find_one_assignment);
    case ORDER_CONFLICT:
      return bind(Controller::conflict_solve_loop<SearchAlgorithm, Vars, Propogator>, stateObj, next_search, order, vars, prop);
    default:
    abort();
  }
}

  template<typename VarValOrder, typename Propogator>
function<void (void)> solve(StateObj* stateObj, const function<void (void)>& next_search, SearchOrder order_in, 
                                 VarValOrder& search_order, CSPInstance& instance, Propogator prop)
{
  typedef typename VarValOrder::first_type::value_type VarType;

  switch(order_in.order)
  {
    case ORDER_STATIC:
    {
      Controller::VariableOrder<VarType, Controller::SlowStaticBranch> 
        order(stateObj, search_order.first, search_order.second);

      return solve_select_search(stateObj, next_search, order_in, order, search_order.first, instance, prop);
    }
    break;
    case ORDER_SDF:
    {
      Controller::VariableOrder<VarType, Controller::SDFBranch> 
        order(stateObj, search_order.first, search_order.second);

      return solve_select_search(stateObj, next_search, order_in, order, search_order.first, instance, prop);
    }
    break;
    case ORDER_SRF:
    {
      Controller::VariableOrder<VarType, Controller::SRFBranch> 
        order(stateObj, search_order.first, search_order.second);

      return solve_select_search(stateObj, next_search, order_in, order, search_order.first, instance, prop); 
    }
    break;
    case ORDER_LDF:
    {
      Controller::VariableOrder<VarType, Controller::LDFBranch> 
        order(stateObj, search_order.first, search_order.second);

      return solve_select_search(stateObj, next_search, order_in, order, search_order.first, instance, prop);
    }
    break;

    case ORDER_ORIGINAL:
    {  
      Controller::VariableOrder<VarType, Controller::StaticBranch>
        order(stateObj, search_order.first, search_order.second);
      return solve_select_search(stateObj, next_search, order_in, order, search_order.first, instance, prop);
    }
    break;
    case ORDER_CONFLICT:
    {
      Controller::VariableOrder<VarType, Controller::StaticBranch>
        order(stateObj, search_order.first, search_order.second);
      return solve_select_search(stateObj, next_search, order_in, order, search_order.first, instance, prop); 
    }
    break;

    case ORDER_WDEG:
    {
#ifdef WDEG
      Controller::VariableOrder<VarType, Controller::WdegBranch>
        order(stateObj, search_order.first, search_order.second);
      return solve_select_search(stateObj, next_search, order_in, order, search_order.first, instance, prop); 
#else
    FAIL_EXIT("This copy of Minion compiled without 'WDEG' support");
#endif
    }
    break;
      

    case ORDER_DOMOVERWDEG:
    {
#ifdef WDEG
      Controller::VariableOrder<VarType, Controller::DomOverWdegBranch>
        order(stateObj, search_order.first, search_order.second);
      return solve_select_search(stateObj, next_search, order_in, order, search_order.first, instance, prop);
#else
    FAIL_EXIT("This copy of Minion compiled without 'WDEG' support");
#endif
    }
    break;
      
    default:
    FAIL_EXIT();
  } 
}

