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

#include "minion.h"


void dump_searchorder(StateObj* state, const SearchOrder& order, ostream& os)
{

    vector<SysInt> non_assigned_vars;
    for(int i = 0; i < order.var_order.size(); ++i)
    {
        AnyVarRef v = BuildCon::get_AnyVarRef_from_Var(state, order.var_order[i]);
        if(!v.isAssigned())
            non_assigned_vars.push_back(i);
    }

    if(non_assigned_vars.size() == 0)
        return;

    os << "VARORDER ";
    if(order.find_one_assignment)
        os << "AUX ";

    switch(order.order)
    {
#define Z(x) case ORDER_##x: os << #x << " "; break;
        Z(STATIC) Z(SDF) Z(SRF) Z(LDF) Z(ORIGINAL)
        Z(WDEG) Z(CONFLICT) Z(DOMOVERWDEG)
#undef Z
        default: abort();
    }


    os << "[";
    bool first=true;
    for(int i = 0; i < non_assigned_vars.size(); ++i)
    {
        AnyVarRef v = BuildCon::get_AnyVarRef_from_Var(state, order.var_order[non_assigned_vars[i]]);
        D_ASSERT(!v.isAssigned());
        if(first) first=false; else os << ",";
        os << v.getBaseVar().get_name();
    }
    os << "]\n";

    os << "VALORDER [";
    first=true;
    for(int i = 0; i < non_assigned_vars.size(); ++i)
    {
        if(first) first=false; else os << ",";
        switch(order.val_order[non_assigned_vars[i]])
        {
            case VALORDER_ASCEND:
                os << "a"; break;
            case VALORDER_DESCEND:
                os << "d"; break;
            case VALORDER_RANDOM:
                os << "r"; break;
            default: abort();
        }
    }
    os << "]\n";


}

void dump_solver(StateObj* state, ostream& os)
{
    os << "# Redumped during search" << endl;
    os << "MINION 3" << endl;
    os << "**VARIABLES**" << endl;
    VariableContainer& vc = getVars(state);

    // booleans;
    for(UnsignedSysInt i = 0; i < vc.boolVarContainer.var_count(); ++i)
    {
        BoolVarRef bv = vc.boolVarContainer.get_var_num(i);
        if(!bv.isAssigned())
            os << "BOOL " << bv.getBaseVar().get_name() << endl;
    }

    // bound vars
    for(UnsignedSysInt i = 0; i < vc.boundVarContainer.var_count(); ++i)
    {
        BoundVarRef bv = vc.boundVarContainer.get_var_num(i);
        if(!bv.isAssigned())
        {
            os << "BOUND " << bv.getBaseVar().get_name() << " ";
            os << "{" << bv.getMin() << ".." << bv.getMax() << "}" << endl;
        }
    }

    // bigRangeVar
    for(UnsignedSysInt i = 0; i < vc.bigRangeVarContainer.var_count(); ++i)
    {
        BigRangeVarRef bv = vc.bigRangeVarContainer.get_var_num(i);
        if(!bv.isAssigned())
        {
            os << "DISCRETE " << bv.getBaseVar().get_name() << " ";
            os << "{" << bv.getMin() << ".." << bv.getMax() << "}" << endl;
        }
    }
    

    // sparseBound
    for(UnsignedSysInt i = 0; i < vc.sparseBoundVarContainer.var_count(); ++i)
    {
        SparseBoundVarRef bv = vc.sparseBoundVarContainer.get_var_num(i);
        vector<DomainInt> dom = vc.sparseBoundVarContainer.get_raw_domain(i);
        if(!bv.isAssigned())
        {

            os << "SPARSEBOUND " << bv.getBaseVar().get_name() << " ";
            os << "{" ;
            bool first = true;
            for(size_t j = 0; j < dom.size(); ++j)
            {
                if(bv.inDomain(dom[j]))
                {
                    if(first)
                        first = false;
                    else
                        os << ",";
                    os << dom[j];
                }
            }
            os << "}" << endl;
        }
    }

    // tuples
    os << "**TUPLELIST**" << endl;
    SearchState& search_state = getState(state);

    for(UnsignedSysInt i = 0; i < search_state.getTupleListContainer()->size(); ++i)
    {
        TupleList* tl = search_state.getTupleListContainer()->getTupleList(i);
        os << tl->getName() << " " << tl->size() << " " << tl->tuple_size() << "\n";
        for(SysInt i = 0; i < tl->size() * tl->tuple_size(); ++i)
        {
            os << (tl->getPointer())[i] << " ";
        }
        os << endl;
    }

    os << "**SEARCH**" << endl;
    if(getState(state).getRawOptimiseVar() &&
        !(getState(state).getRawOptimiseVar()->isAssigned()))
    {
        if(getState(state).isMaximise())
            os << "MAXIMISING ";
        else
            os << "MINIMISING ";
        os << getState(state).getRawOptimiseVar()->getBaseVar().get_name() << "\n";
    }
    os << "PRINT ";
    os << ConOutput::print_vars(getState(state).getPrintMatrix());
    os << endl;


    for(UnsignedSysInt i = 0; i < getState(state).getInstance()->search_order.size(); ++i)
    {
        dump_searchorder(state, getState(state).getInstance()->search_order[i], os);
    }

    os << "**CONSTRAINTS**" << endl;
    for(UnsignedSysInt i = 0; i < search_state.getConstraintList().size(); ++i)
    {
        os << search_state.getConstraintList()[i]->full_output_name() << "\n";
    }
    
    os << "**EOF**" << endl;
}

void dump_solver(StateObj* state, string filename)
{
    ofstream ofs(filename.c_str());
    dump_solver(state, ofs);
}
