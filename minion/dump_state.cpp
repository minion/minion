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

    
}