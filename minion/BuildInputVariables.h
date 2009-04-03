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

#ifndef BUILDINPUTCONSTRAINT_H
#define BUILDINPUTCONSTRAINT_H

#include "system/system.h"

#include "variables/AnyVarRef.h"

#include "inputfile_parse/CSPSpec.h"

namespace BuildCon
{

/// Create all the variables used in the CSP.
void build_variables(StateObj* stateObj, const ProbSpec::VarContainer& vars);


/// Build the variable and value ordering used.
/// The var order is placed, the val order is returned.
pair<vector<AnyVarRef>, vector<int> > build_val_and_var_order(StateObj* stateObj, SearchOrder instance);

}

#endif

