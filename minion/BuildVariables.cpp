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

namespace BuildCon
{

vector<AnyVarRef>
get_AnyVarRef_from_Var(StateObj* stateObj, const vector<Var>& vec)
{
  vector<AnyVarRef> ret_vec;
  ret_vec.reserve(vec.size());

  for(SysInt i = 0; i < (SysInt)vec.size(); ++i)
    ret_vec.push_back(get_AnyVarRef_from_Var(stateObj, vec[i]));

  return ret_vec;
}

/// Helper function used in a few places.
AnyVarRef
get_AnyVarRef_from_Var(StateObj* stateObj, Var v)
{
  switch(v.type())
        {
          case VAR_BOOL:
            return AnyVarRef(getVars(stateObj).boolVarContainer.get_var_num(v.pos()));
          case VAR_NOTBOOL:
          return AnyVarRef(VarNotRef(getVars(stateObj).boolVarContainer.get_var_num(v.pos())));
          case VAR_BOUND:
            return AnyVarRef(getVars(stateObj).boundVarContainer.get_var_num(v.pos()));
          case VAR_SPARSEBOUND:
            return AnyVarRef(getVars(stateObj).sparseBoundVarContainer.get_var_num(v.pos()));
          case VAR_DISCRETE:
            return AnyVarRef(getVars(stateObj).bigRangeVarContainer.get_var_num(v.pos()));
          case VAR_SPARSEDISCRETE:
            INPUT_ERROR("Sparse Discrete not supported at present");
          case VAR_CONSTANT:
            return AnyVarRef(ConstantVar(stateObj, v.pos()));
          default:
            INPUT_ERROR("Unknown variable type " << v.type() << ".");
        }
}



  /// Create all the variables used in the CSP.
  void build_variables(StateObj* stateObj, const ProbSpec::VarContainer& vars)
  {
    getVars(stateObj).boolVarContainer.setVarCount(vars.BOOLs);
    getVars(stateObj).boundVarContainer.addVariables(vars.bound);
    getVars(stateObj).sparseBoundVarContainer.addVariables(vars.sparse_bound);
    getVars(stateObj).bigRangeVarContainer.addVariables(vars.discrete);

    for(SysInt i = 0; i < (SysInt)vars.sparse_discrete.size(); ++i)
    { INPUT_ERROR("Sparse discrete disabled at present due to bugs. Sorry."); }
  }

}
