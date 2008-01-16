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
  
DEFCON2(LexLeqCon);
DEFCON2(LexLessCon);

DEFCON2(GreaterEqualSumCon);

Constraint*
MakeGreaterEqualSumCon(const vector<BoolVarRef>& t1, const ConstantVar& sum, ConstraintBlob& b, const BoolVarRef& reifyVar)
{ 
  runtime_val t2(sum.getAssignedValue());
  FUNBODY2(BoolGreaterEqualSumCon);
} 

DEFCON2(LessEqualSumCon);

Constraint*
MakeLessEqualSumCon(const vector<BoolVarRef>& t1, const ConstantVar& sum, ConstraintBlob& b, const BoolVarRef& reifyVar)
{ 
  runtime_val t2(sum.getAssignedValue());
  FUNBODY2(BoolLessEqualSumCon); 
}

DEFCON3(ProductCon);
Constraint*
MakeProductCon(const BoolVarRef& t1,const BoolVarRef& t2,const BoolVarRef& t3, ConstraintBlob& b, const BoolVarRef& reifyVar)
{ FUNBODY3(AndCon); }

DEFCON3(ElementCon);
DEFCON3(GACElementCon);

template<>
struct BuildConstraint<3, 0>
{
  template<typename T1, typename T2>
  static 
  Constraint* build(const pair<pair<EmptyType, vector<T1>* >, vector<T2>* >& vars, ConstraintBlob& b, int)
  { 
	BoolVarRef reifyVar;
	if(b.reified || b.implied_reified)
	  reifyVar = boolean_container.get_var_num(b.reify_var.pos);
	switch(b.constraint.type)
	{
	  case CT_LEXLEQ:
		return MakeLexLeqCon(*(vars.first.second), *(vars.second), b, reifyVar);
	  case CT_LEXLESS:
		return MakeLexLessCon(*(vars.first.second), *(vars.second), b, reifyVar);
	  case CT_LEQSUM:
		return MakeLessEqualSumCon(*(vars.first.second), vars.second->at(0), b, reifyVar);
	  case CT_GEQSUM:
		return MakeGreaterEqualSumCon(*(vars.first.second), vars.second->at(0), b, reifyVar);
	  case CT_PRODUCT2:
		return MakeProductCon(vars.first.second->at(0), vars.first.second->at(1), vars.second->at(0), b, reifyVar);
	  case CT_ELEMENT:
	  {
	    // the assignval is kept in the initial array for safe keeping. Have to dig it out!
	    T1 assignval = vars.first.second->back();
		vector<T1> t1 = *(vars.first.second);
		t1.pop_back();
		return MakeElementCon(t1, vars.second->at(0), assignval, b, reifyVar);
	  }
	  case CT_GACELEMENT:
	  {
	    // the assignval is kept in the initial array for safe keeping. Have to dig it out!
	    T1 assignval = vars.first.second->back();
		vector<T1> t1 = *(vars.first.second);
		t1.pop_back();
		return MakeGACElementCon(t1, vars.second->at(0), assignval, b, reifyVar);
	  }
	  
		
	  default:
		D_FATAL_ERROR("unsupported constraint");			
	}
  }
};

Constraint*
build_constraint_binary(ConstraintBlob& b)
{ return BuildConstraint<3, 2>::build(EmptyType(), b, 0); }
}



