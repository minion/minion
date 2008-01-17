/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/*
 *  BuildConstraintConstructs.h
 *  cutecsp
 *
 *  Created by Chris Jefferson on 14/04/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */


#include "BuildConstraint.h"

#ifdef CONSTRAINT_TYPE
#undef CONSTRAINT_TYPE
#endif
#ifdef DYNAMIC_BUILD_CONSTRAINT
// QUICK_COMPILE doesn't work on dynamic constraints.
#undef QUICK_COMPILE
#define CONSTRAINT_TYPE DynamicConstraint
#else
#ifdef STATIC_BUILD_CONSTRAINT
#define CONSTRAINT_TYPE Constraint
#else
#error BuildConstraintConstructs.h is not a normal header. Error occured.
#endif
#endif

#define MERGE2(x,y) x ## y
#define MERGE(x , y) MERGE2(x,y)
#define BUILDCON MERGE(Build , CONSTRAINT_TYPE)
 
namespace BuildCon
{  

/// General case in iteratively build constraints.
/// This isn't inline, as we don't want the compiler to waste time inlining it.
template<int initial_size, int size>
struct BUILDCON
{
  template<typename ConData>
  static 
  CONSTRAINT_TYPE* build(const ConData& partial_build, ConstraintBlob& b, int pos);
};


template<int initial_size, int size>
template<typename ConData>
CONSTRAINT_TYPE* 
BUILDCON<initial_size, size>::
build(const ConData& partial_build, ConstraintBlob& b, int pos)
{
  const vector<Var>& vars = b.vars[pos];
  int type = vars[0].type;
  BOOL same_type = true;
  for(unsigned i = 1; i < vars.size(); ++i)
  {
	if(vars[i].type != type)
	{
	  same_type = false;
	  break;
	}
  }
  #ifndef QUICK_COMPILE
  if(same_type)
  {
	switch(type)
	{
	  case VAR_BOOL:
	  {
		light_vector<BoolVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = boolean_container.get_var_num(vars[i].pos);
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }
	  case VAR_NOTBOOL:
	  {
		light_vector<VarNot<BoolVarRef> > v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = VarNotRef(boolean_container.get_var_num(vars[i].pos));
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }
	  case VAR_BOUND:
	  {
		light_vector<BoundVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = boundvar_container.get_var_num(vars[i].pos);
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }		
	  case VAR_SPARSEBOUND:
	  {
		light_vector<SparseBoundVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = sparse_boundvar_container.get_var_num(vars[i].pos);
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }
	  case VAR_DISCRETE:
	  {
		light_vector<LRangeVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = rangevar_container.get_var_num(vars[i].pos);
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }		
	  case VAR_LONG_DISCRETE:
	  {
		light_vector<BigRangeVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = big_rangevar_container.get_var_num(vars[i].pos);
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }
	  case VAR_SPARSEDISCRETE:	
		D_FATAL_ERROR( "Sparse Discrete Variables current broken. Sorry");
		
	  case VAR_CONSTANT:
	  {
		light_vector<ConstantVar> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = ConstantVar(vars[i].pos);
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }
	}
  }
  else
  #endif
  {
	light_vector<AnyVarRef> v(vars.size());
	for(unsigned i = 0; i < vars.size(); ++i)
	  v[i] = get_AnyVarRef_from_Var(vars[i]);
	
	return BUILDCON<initial_size, size - 1>::
	  build(make_pair(partial_build, &v), b, pos + 1);
  }
  // This FAIL_EXIT is here to stop a "no return in non-void function" warning. It should never be reached.
  D_FATAL_ERROR( "This should never be reached..");
}  



}



