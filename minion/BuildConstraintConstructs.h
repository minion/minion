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


#define MERGE2(x,y) x ## y
#define MERGE(x , y) MERGE2(x,y)
#define BUILDCON BuildConObj
 
namespace BuildCon
{  

/// General case in iteratively build constraints.
/// This isn't inline, as we don't want the compiler to waste time inlining it.
template<int initial_size, int size>
struct BUILDCON
{
  template<typename ConData>
  static 
  AbstractConstraint* build(StateObj* stateObj, const ConData& partial_build, ConstraintBlob& b, int pos) _NOINLINE;
};


template<int initial_size, int size>
template<typename ConData>
AbstractConstraint* 
BUILDCON<initial_size, size>::
build(StateObj* stateObj, const ConData& partial_build, ConstraintBlob& b, int pos)
{
  const vector<Var>& vars = b.vars[pos];
  int type = vars[0].type();
  bool same_type = true;
  for(unsigned i = 1; i < vars.size(); ++i)
  {
	if(vars[i].type() != type)
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
		  v[i] = getVars(stateObj).getBooleanContainer().get_var_num(vars[i].pos());
		return BUILDCON<initial_size, size - 1>::
		  build(stateObj, make_pair(partial_build, &v), b, pos + 1);
	  }
	  case VAR_NOTBOOL:
	  {
		light_vector<VarNot<BoolVarRef> > v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = VarNotRef(getVars(stateObj).getBooleanContainer().get_var_num(vars[i].pos()));
		return BUILDCON<initial_size, size - 1>::
		  build(stateObj, make_pair(partial_build, &v), b, pos + 1);
	  }
	  case VAR_BOUND:
	  {
		light_vector<BoundVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = getVars(stateObj).getBoundvarContainer().get_var_num(vars[i].pos());
		return BUILDCON<initial_size, size - 1>::
		  build(stateObj, make_pair(partial_build, &v), b, pos + 1);
	  }		
	  case VAR_SPARSEBOUND:
	  {
		light_vector<SparseBoundVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = getVars(stateObj).getSparseBoundvarContainer().get_var_num(vars[i].pos());
		return BUILDCON<initial_size, size - 1>::
		  build(stateObj, make_pair(partial_build, &v), b, pos + 1);
	  }
      case VAR_DISCRETE:
	  {
		light_vector<BigRangeVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = getVars(stateObj).getBigRangevarContainer().get_var_num(vars[i].pos());
		return BUILDCON<initial_size, size - 1>::
		  build(stateObj, make_pair(partial_build, &v), b, pos + 1);
	  }
	  case VAR_SPARSEDISCRETE:	
		INPUT_ERROR( "Sparse Discrete Variables current broken. Sorry");
		
	  case VAR_CONSTANT:
	  {
		light_vector<ConstantVar> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = ConstantVar(stateObj, vars[i].pos());
		return BUILDCON<initial_size, size - 1>::
		  build(stateObj, make_pair(partial_build, &v), b, pos + 1);
	  }
	}
  }
  else
  #endif
  {
	light_vector<AnyVarRef> v(vars.size());
	for(unsigned i = 0; i < vars.size(); ++i)
	  v[i] = get_AnyVarRef_from_Var(stateObj, vars[i]);
	
	return BUILDCON<initial_size, size - 1>::
	  build(stateObj, make_pair(partial_build, &v), b, pos + 1);
  }
  // This FAIL_EXIT is here to stop a "no return in non-void function" warning. It should never be reached.
  INPUT_ERROR( "This should never be reached..");
}  



}



