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

#define NO_MAIN
#include "minion.h"
#include "CSPSpec.h"
using namespace ProbSpec;

#include "BuildConstraintConstructs.h"

namespace BuildCon
{
  
  DEFCON3(GeqWeightBoolSumCon);
  DEFCON3(LeqWeightBoolSumCon);
  DEFCON2(GreaterEqualSumCon);
  DEFCON2(LessEqualSumCon);
  
  template<>
	struct BuildConstraint<4, 0>
  {
	
	/// Special case for weighted sum constraints.
	template<typename T1, typename T2>
	static
	Constraint* build(const pair<pair<pair<EmptyType, vector<int>* >, vector<T1>* >, vector<T2>* >& vars, ConstraintBlob& b, int)
  { 
	  BoolVarRef reifyVar;
	  if(b.reified || b.implied_reified)
		reifyVar = boolean_container.get_var_num(b.reify_var.pos);
	  
	  const vector<int>& scale = *(vars.first.first.second);
	  const vector<T1>& vec = *(vars.first.second);
	  const T2& mult_val = vars.second->at(0);
	  
	  bool multipliers_size_one = true;
	  for(unsigned i = 0; i < scale.size(); ++i)
	  {
	    if(scale[i] != 1 && scale[i] != -1)
		{
		  multipliers_size_one = false;
		  i = scale.size();
		}
	  }
	  
	  if(multipliers_size_one)
	  {
		vector<SwitchNeg<T1> > mult_vars;
		for(unsigned int i = 0; i < vec.size(); ++i)
		  mult_vars.push_back(SwitchNeg<T1>(vec[i], scale[i]));
		
		if(b.constraint.type == CT_WEIGHTLEQSUM)
		  return MakeLessEqualSumCon(mult_vars, mult_val, b, reifyVar);
		if(b.constraint.type == CT_WEIGHTGEQSUM)
		  return MakeGreaterEqualSumCon(mult_vars, mult_val, b, reifyVar);
	  }
	  else
	  {
		vector<MultiplyVar<T1> > mult_vars;
		for(unsigned int i = 0; i < vec.size(); ++i)
		  mult_vars.push_back(MultiplyVar<T1>(vec[i], scale[i]));
		
		if(b.constraint.type == CT_WEIGHTLEQSUM)
		  return MakeLessEqualSumCon(mult_vars, mult_val, b, reifyVar);
		if(b.constraint.type == CT_WEIGHTGEQSUM)
		  return MakeGreaterEqualSumCon(mult_vars, mult_val, b, reifyVar);
	  }
	  
	  // We should never get here!  
	  throw new parse_exception("Serious internal error.");
  }
	
	/// Special case for weighted sum constraints on booleans.
	template<typename T2>
	static
	Constraint* build(const pair<pair<pair<EmptyType, vector<int>* >, vector<BoolVarRef>* >, vector<T2>* >& vars, ConstraintBlob& b, int)
  { 
	  BoolVarRef reifyVar;
	  if(b.reified || b.implied_reified)
		reifyVar = boolean_container.get_var_num(b.reify_var.pos);
	  
	  const vector<int>& scale = *(vars.first.first.second);
	  const vector<BoolVarRef>& vec = *(vars.first.second);
	  const T2& mult_val = vars.second->at(0);
	  
	  if(b.constraint.type == CT_WEIGHTLEQSUM)
		return MakeLeqWeightBoolSumCon(vec, scale, mult_val, b, reifyVar);
	  if(b.constraint.type == CT_WEIGHTGEQSUM)
		return MakeGeqWeightBoolSumCon(vec, scale, mult_val, b, reifyVar);
	  
	  // We should never get here!  
	  D_FATAL_ERROR( "unsupported constraint");	
  }
  };
  
  Constraint*
	build_constraint_weighted_sum(ConstraintBlob& b)
  {
	  // We have to special case the inital array of constants, so we pick them off
	  // manually, and remove them from the constraint. Then the normal framework
	  // deals with everything else.
	  vector<int> weights;
	  const vector<Var>& vars = b.vars[0];
	  for(unsigned i = 0; i < vars.size(); ++i)
		weights.push_back(vars[i].pos);
	  b.vars.erase(b.vars.begin());
	  // We make the first value 100, just so it is easy to seperate this case from others
	  return BuildConstraint<4, 2>::build(make_pair(EmptyType(), &weights), b, 0);
  }
}


