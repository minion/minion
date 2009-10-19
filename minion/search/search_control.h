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
    
    VariableOrder* make_search_order(SearchOrder order, StateObj* stateObj)
    {
        // collect the variables in the SearchOrder object 
        vector<AnyVarRef>* var_array= new vector<AnyVarRef>();
        for(int i=0; i<order.var_order.size(); i++)
        {
            var_array->push_back(get_AnyVarRef_from_Var(stateObj, order.var_order[i]));
            // some check here?
        }
        
        VariableOrder* vo;
        VariableOrder* vo2;
        
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
        case ORDER_SRF:
            vo=new SRFBranch(*var_array, order.val_order, stateObj);
            break;
        case ORDER_LDF:
            vo=new LDFBranch(*var_array, order.val_order, stateObj);
            break;
        case ORDER_CONFLICT:
            // for the time being, just use static as the underlying order
            vo2=new StaticBranch(*var_array, order.val_order, stateObj);
            vo=new ConflictBranch(*var_array, order.val_order, vo2, stateObj);
            break;
        
        #ifdef WDEG
        case ORDER_WDEG:
            vo=new WdegBranch(*var_array, order.val_order, stateObj);
            break;
        case ORDER_DOMOVERWDEG:
            vo=new DomOverWdegBranch(*var_array, order.val_order, stateObj);
            break;
        #endif
        
        default:
            cout << "Order not found in make_search_order." << endl;
            abort();
        }
        return vo;
    }
    
    VariableOrder* make_search_order_multiple(vector<SearchOrder> order, StateObj* stateObj)
    {
        VariableOrder* vo;
        
        if(order.size()==1)
        {
            return make_search_order(order[0], stateObj);
        }
        else
        {
            vector<VariableOrder*> vovector;
            for(int i=0; i<order.size(); i++)
            {
                vovector.push_back(make_search_order(order[i], stateObj));
                if(order[i].find_one_assignment && i!= order.size()-1)
                {
                    cout << "Only one VARORDER AUX is allowed, and it must be the final VARORDER command." << endl;
                    abort();
                }
            }
            
            vo=new MultiBranch(vovector, stateObj);
        }
        
        return vo;
    }
    
    
// returns an instance of SearchManager with the required variable ordering, propagator etc.
SearchManager* make_search_manager(StateObj* stateObj, PropagationLevel prop_method, vector<SearchOrder> order)
{
    VariableOrder* vo;
    
    vo=make_search_order_multiple(order, stateObj);
    
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
    
    vector<AnyVarRef>* all_vars=new vector<AnyVarRef>();
    
    for(int i=0; i<order.size(); i++)
    {
        for(int j=0; j<order[i].var_order.size(); j++)
        {
            all_vars->push_back(get_AnyVarRef_from_Var(stateObj, order[i].var_order[j]));
        }
    }
    
    // need to switch here for different search algorthms. plain, parallel, group
    SearchManager* sm=new SearchManager(stateObj, *all_vars, order, vo, p);
    
    return sm;
}

}
