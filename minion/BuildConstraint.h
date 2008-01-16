/* Minion
* Copyright (C) 2006
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

#ifndef BUILDCONSTRAINT_H
#define BUILDCONSTRAINT_H

namespace BuildCon
{  
  
  enum VarType
{
  VAR_ARRAY = -100,
  SINGLE_VAR = -101
};


/// Helper function used in a few places.
AnyVarRef
get_AnyVarRef_from_Var(Var v);

/// Create all the variables used in the CSP.
void build_variables(const ProbSpec::VarContainer& vars);


/// Build the variable and value ordering used.
/// The var order is placed, the val order is returned.
pair<vector<AnyVarRef>, vector<BOOL> > build_val_and_var_order( ProbSpec::CSPInstance& instance);

}
#ifdef DYNAMICTRIGGERS
DynamicConstraint*
build_dynamic_constraint(ConstraintBlob& b);
#endif

Constraint*
build_constraint(ConstraintBlob& b);

#endif
