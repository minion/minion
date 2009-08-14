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

//#include <boost/bind.hpp>
//using boost::bind;

#include "../system/system.h"
#include "SearchManager.h"
#include "variable_orderings.h"

namespace Controller
{

// returns an instance of SearchManager with the required variable ordering, propagator etc.
SearchManager* make_search_manager(StateObj* stateObj, PropagationLevel prop_method, SearchOrder order)
{
    cout << "Entered make_search_manager." << endl;
    
    // collect the variables in the SearchOrder object 
    vector<AnyVarRef>* var_array= new vector<AnyVarRef>();
    for(int i=0; i<order.var_order.size(); i++)
    {
        var_array->push_back(get_AnyVarRef_from_Var(stateObj, order.var_order[i]));
        // some check here?
    }
    
    VariableOrder* vo;
    
    switch(order.order)  // get the VarOrderEnum
    {
    case ORDER_STATIC:
        vo=new StaticBranch(*var_array, order.val_order, stateObj);
        break;
    case ORDER_ORIGINAL:
        vo=new StaticBranch(*var_array, order.val_order, stateObj);
        break;
    case ORDER_SDF:
        vo=new SDFBranch(*var_array, order.val_order, stateObj);
        break;
    default:
        cout << "Order not found in make_search_manager." << endl;
        abort();
    }
    
    cout << "Made VariableOrder object." <<endl;
    
    Propagate * p;
    switch(prop_method)
    {   // doesn't cover the PropLevel_None case.
    case PropLevel_GAC:
        p= new PropGAC();
        break;
    case PropLevel_SAC:
        p= new PropSAC();
        break;
    case PropLevel_SSAC:
        p=new PropSSAC();
        break;
    case PropLevel_SACBounds:
        p=new PropSAC_Bounds();
        break;
    case PropLevel_SSACBounds:
        p=new PropSSAC_Bounds();
        break;
    default:
        cout << "Propagation method not found in make_search_manager." << endl;
        abort();
    }
    
    cout << "Made Propagate object." <<endl;
    
    // need to switch here for different search algorthms. plain, parallel, group or conflict
    SearchManager* sm=new SearchManager(stateObj, *var_array, vo, p);
    
    cout << "Exiting make_search_manager"<<endl;
    return sm;
}

}
