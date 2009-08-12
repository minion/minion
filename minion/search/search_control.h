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

// returns an instance of SearchManager with the required variable ordering, propagator etc.
SearchManager& make_search_manager(StateObj* stateObj, PropagationLevel prop_method, SearchOrder order)
{
    VariableOrder* vo;
    
    switch(order.order)  // get the VarOrderEnum
    {
    case ORDER_STATIC:
        vo=new StaticBranch(order.var_order, order.val_order, stateObj);
        break;
    case ORDER_SDF:
        vo=new SDFBranch(order.var_order, order.val_order, stateObj);
        break;
    default:
        abort();
    }
    
    Propagator* p;
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
        abort();
    }
    
    // need to switch here for different search algorthms. plain, parallel, group or conflict
    SearchManager& sm=new SearchManager(stateObj, order.var_array, vo, p);
    return sm;
}

