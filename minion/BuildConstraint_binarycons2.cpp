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
  
  template<typename T1, typename T2>
  Constraint*
  MakeEqualCon(const T1& t1, const T2& t2, ConstraintBlob& b, const BoolVarRef& reifyVar)
{
	if(b.reified)
	  return ReifiedEqualCon(t1, t2, reifyVar);
	else if(b.implied_reified)
	  return truereifyCon(EqualCon(t1,t2), reifyVar);
	else
	  return EqualCon(t1, t2);
}

  template<typename T1, typename T2>
  Constraint*
  MakeEqualMinusCon(const T1& t1, const T2& t2, ConstraintBlob& b, const BoolVarRef& reifyVar)
{
	if(b.reified)
	  return ReifiedEqualMinusCon(t1, t2, reifyVar);
	else if(b.implied_reified)
	  return truereifyCon(EqualMinusCon(t1,t2), reifyVar);
	else
	  return EqualMinusCon(t1, t2);
}


template<typename T1>
Constraint*
MakeEqualCon(const T1& t1, const ConstantVar& t2, ConstraintBlob& b, const BoolVarRef& reifyVar)
{
  if(b.reified)
	return ReifiedEqualCon(t1, t2, reifyVar);
  else if(b.implied_reified)
	return truereifyCon(EqualCon(t1,t2), reifyVar);
  else
	return UnaryEqualCon(t1, runtime_val(t2.getAssignedValue()));
}

DEFCON2(NeqCon);

template<typename T1>
Constraint*
MakeNeqCon(const T1& t1, const ConstantVar& t2, ConstraintBlob& b, const BoolVarRef& reifyVar)
{
  if(b.reified)
	return reifyCon(UnaryNeqCon(t1, runtime_val(t2.getAssignedValue())), reifyVar);
  else if(b.implied_reified)
	return truereifyCon(UnaryNeqCon(t1, runtime_val(t2.getAssignedValue())), reifyVar);
  else
	return UnaryNeqCon(t1, runtime_val(t2.getAssignedValue()));
}

DEFCON2(MinCon);
DEFCON2(MaxCon);
DEFCON3(LeqCon);

template<>
struct BuildConstraint<2, 0>
{
  template<typename T1, typename T2>
  static 
  Constraint* build(const pair<pair<EmptyType, vector<T1>* >, vector<T2>* >& vars, ConstraintBlob& b, int pos)
  { 
	BoolVarRef reifyVar;
	if(b.reified || b.implied_reified)
	  reifyVar = boolean_container.get_var_num(b.reify_var.pos);
	D_ASSERT(pos == static_cast<int>(b.vars.size()) || b.constraint.type == CT_INEQ || b.constraint.type == CT_OCCURRENCE);
	switch(b.constraint.type)
	{
	  case CT_MINUSEQ:
	    return MakeEqualMinusCon(vars.first.second->at(0), vars.second->at(0), b, reifyVar);
	  case CT_EQ:
	    return MakeEqualCon(vars.first.second->at(0), vars.second->at(0), b, reifyVar);
		// Special case: we know the last element of an ineq is always a constant!
	  case CT_INEQ:
		D_ASSERT(b.vars[2].size() == 1 && b.vars[2][0].type == VAR_CONSTANT);
	    return MakeLeqCon(vars.first.second->at(0), vars.second->at(0), runtime_val(b.vars[2][0].pos), b, reifyVar);
	  case CT_DISEQ:
	    return MakeNeqCon(vars.first.second->at(0), vars.second->at(0), b, reifyVar);	
	  case CT_MIN:
	    if(vars.first.second->at(0).getInitialMin() == 0 && vars.first.second->at(0).getInitialMax() == 1 &&
	       vars.first.second->at(1).getInitialMin() == 0 && vars.first.second->at(1).getInitialMax() == 1 &&
	       vars.second->at(0).getInitialMin() == 0 && vars.second->at(0).getInitialMax() == 1)
	      return AndCon(vars.first.second->at(0), vars.first.second->at(1), vars.second->at(0));
	    else
		  return MakeMinCon(*(vars.first.second), vars.second->at(0), b, reifyVar);
	  case CT_MAX:
		return MakeMaxCon(*(vars.first.second), vars.second->at(0), b, reifyVar);
	 		
	  default:
		D_FATAL_ERROR( "unsupported constraint");
		FAIL_EXIT();			
	}
  }
};

Constraint*
build_constraint_binary2(ConstraintBlob& b)
{ return BuildConstraint<2, 2>::build(EmptyType(), b, 0); }

}


