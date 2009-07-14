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

