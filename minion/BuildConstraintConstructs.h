/*
 *  BuildConstraintConstructs.h
 *  cutecsp
 *
 *  Created by Chris Jefferson on 14/04/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

#include "BuildConstraint.h"

#ifdef DYNAMIC_BUILD_CONSTRAINT
#define CONSTRAINT_TYPE DynamicConstraint
#else
#define CONSTRAINT_TYPE Constraint
#endif

#define MERGE2(x,y) x ## y
#define MERGE(x , y) MERGE2(x,y)
#define BUILDCON MERGE(Build , CONSTRAINT_TYPE)
 
namespace BuildCon
{  

/// General case in iteratively build constraints.
/// This isn't inline void, as we don't want the compiler to waste time inlining it.
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
  bool same_type = true;
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
		vector<BoolVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = boolean_container.get_var_num(vars[i].pos);
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }
	  case VAR_NOTBOOL:
	  {
		vector<VarNot<BoolVarRef> > v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = VarNotRef(boolean_container.get_var_num(vars[i].pos));
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }
	  case VAR_BOUND:
	  {
		vector<BoundVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = boundvar_container.get_var_num(vars[i].pos);
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }		
	  case VAR_SPARSEBOUND:
	  {
		vector<SparseBoundVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = sparse_boundvar_container.get_var_num(vars[i].pos);
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }
	  case VAR_DISCRETE:
	  {
		vector<LRangeVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = rangevar_container.get_var_num(vars[i].pos);
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }		
	  case VAR_LONG_DISCRETE:
	  {
		vector<BigRangeVarRef> v(vars.size());
		for(unsigned i = 0; i < vars.size(); ++i)
		  v[i] = big_rangevar_container.get_var_num(vars[i].pos);
		return BUILDCON<initial_size, size - 1>::
		  build(make_pair(partial_build, &v), b, pos + 1);
	  }
	  case VAR_SPARSEDISCRETE:	
		D_FATAL_ERROR( "Sparse Discrete Variables current broken. Sorry");
		
	  case VAR_CONSTANT:
	  {
		vector<ConstantVar> v(vars.size());
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
	vector<AnyVarRef> v(vars.size());
	for(unsigned i = 0; i < vars.size(); ++i)
	  v[i] = get_AnyVarRef_from_Var(vars[i]);
	
	return BUILDCON<initial_size, size - 1>::
	  build(make_pair(partial_build, &v), b, pos + 1);
  }
  // This FAIL_EXIT is here to stop a "no return in non-void function" warning. It should never be reached.
  D_FATAL_ERROR( "This should never be reached..");
}  


/**
*  There follows a bunch of nasty code which unfolds the partially constructed constraints made by the function above and imposes them.
 *  This code is annoyingly horrible, but at the moment it (mostly) works, so it's best not to touch it if you can help it.
 *  At some point, this needs neatening so normal people can add constraints to the parser.
 */

#ifdef DYNAMIC_BUILD_CONSTRAINT

#define FUNBODY1(FUNCTION) \
if(b.reified || b.implied_reified) \
{ D_FATAL_ERROR("Cannot reify a watched constraint"); } \
else return FUNCTION(t1);

#define FUNBODY2(FUNCTION) \
if(b.reified || b.implied_reified) \
{ D_FATAL_ERROR("Cannot reify a watched constraint"); } \
else return FUNCTION(t1,t2);

#define FUNBODY3(FUNCTION) \
if(b.reified || b.implied_reified) \
{ D_FATAL_ERROR("Cannot reify a watched constraint"); } \
else return FUNCTION(t1,t2,t3);

#else

#define FUNBODY1(FUNCTION) \
if(b.reified) \
{ return reifyCon(FUNCTION(t1),reifyVar); } \
else if(b.implied_reified) \
{ return truereifyCon(FUNCTION(t1), reifyVar); } \
else \
{ return FUNCTION(t1); }

#define FUNBODY2(FUNCTION) \
if(b.reified) \
{ return reifyCon(FUNCTION(t1,t2),reifyVar); } \
else if(b.implied_reified) \
{ return truereifyCon(FUNCTION(t1,t2), reifyVar); } \
else \
{ return FUNCTION(t1,t2); }\

#define FUNBODY3(FUNCTION) \
if(b.reified) \
{ return reifyCon(FUNCTION(t1,t2,t3),reifyVar); } \
else if(b.implied_reified) \
{ return truereifyCon(FUNCTION(t1,t2,t3), reifyVar); } \
else \
{ return FUNCTION(t1,t2,t3); }\

#endif

#define DEFCON1(CON) \
template<typename T1> \
 CONSTRAINT_TYPE* \
Make ## CON (const T1& t1,  ConstraintBlob& b, const BoolVarRef& reifyVar) \
{ FUNBODY1(CON); }

#define DEFCON2(CON) \
template<typename T1, typename T2> \
CONSTRAINT_TYPE*\
Make ## CON (const T1& t1, const T2& t2, ConstraintBlob& b, const BoolVarRef& reifyVar) \
{ \
  FUNBODY2(CON); \
}

#define DEFCON3(CON) \
template<typename T1, typename T2, typename T3> \
CONSTRAINT_TYPE* \
Make ## CON (const T1& t1, const T2& t2, const T3& t3, ConstraintBlob& b, const BoolVarRef& reifyVar) \
{ FUNBODY3(CON); }


}

