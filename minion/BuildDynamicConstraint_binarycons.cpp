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



// QUICK_COMPILE won't work on this file.
#undef QUICK_COMPILE

#define NO_MAIN
#include "minion.h"
#include "CSPSpec.h"
using namespace ProbSpec;

#ifdef DYNAMICTRIGGERS

#define DYNAMIC_BUILD_CONSTRAINT
#include "BuildConstraintConstructs.h"

namespace BuildCon
{


template<typename T1, typename T2>     
DynamicConstraint*
MakeBoolGreaterEqualSumConDynamic(const T1&, const T2&, ConstraintBlob&, const BoolVarRef&)
{ D_FATAL_ERROR("Watched Sum only supported on sums of booleans to constant!"); }

DynamicConstraint*
MakeBoolGreaterEqualSumConDynamic(const vector<BoolVarRef>& t1, const ConstantVar& sum, ConstraintBlob& b, const BoolVarRef& reifyVar)
{ 
  runtime_val t2(sum.getAssignedValue());
  FUNBODY2(BoolGreaterEqualSumConDynamic);
} 

template<typename T1, typename T2>     
DynamicConstraint*
MakeBoolLessEqualSumConDynamic(const T1&, const T2&, ConstraintBlob&, const BoolVarRef&)
{ D_FATAL_ERROR("Watched Sum only supported on sums of booleans to constant!"); }


DynamicConstraint*
MakeBoolLessEqualSumConDynamic(const vector<BoolVarRef>& t1, const ConstantVar& sum, ConstraintBlob& b, const BoolVarRef& reifyVar)
{ 
  runtime_val t2(sum.getAssignedValue());
  FUNBODY2(BoolLessEqualSumConDynamic); 
}

DEFCON3(ElementConDynamic);
DEFCON2(VecNeqConDynamic);

template<>
struct BuildDynamicConstraint<2, 0>
{
  template<typename T1, typename T2>
  static 
  DynamicConstraint* build(const pair<pair<EmptyType, vector<T1>* >, vector<T2>* >& vars, ConstraintBlob& b, int)
  { 
	BoolVarRef reifyVar;
	if(b.reified || b.implied_reified)
	  reifyVar = boolean_container.get_var_num(b.reify_var.pos);
	switch(b.constraint.type)
	{
	  case CT_WATCHED_LEQSUM:
		return MakeBoolLessEqualSumConDynamic(*(vars.first.second), vars.second->at(0), b, reifyVar);
	  case CT_WATCHED_GEQSUM:
		return MakeBoolGreaterEqualSumConDynamic(*(vars.first.second), vars.second->at(0), b, reifyVar);
	  case CT_WATCHED_VECNEQ:
	    return MakeVecNeqConDynamic(*(vars.first.second), *(vars.second), b, reifyVar);
      case CT_WATCHED_ELEMENT:
	  {
	    // the assignval is kept in the initial array for safe keeping. Have to dig it out!
	    T1 assignval = vars.first.second->back();
		vector<T1> t1 = *(vars.first.second);
		t1.pop_back();
		return MakeElementConDynamic(t1, vars.second->at(0), assignval, b, reifyVar);
	  }
		
	  default:
		D_FATAL_ERROR( "unsupported constraint");
	}
  }
};

DynamicConstraint*
build_dynamic_constraint_binary(ConstraintBlob& b)
{ return BuildDynamicConstraint<2, 2>::build(EmptyType(), b, 0); }
}


#endif

