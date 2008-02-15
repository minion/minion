/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

#define NO_MAIN

//#include "../minion.h"
#include "../CSPSpec.h"

using namespace ProbSpec;

#define STATIC_BUILD_CONSTRAINT
#include "../BuildConstraintConstructs.h"
#define DYNAMIC_BUILD_CONSTRAINT
#include "../BuildConstraintConstructs.h"

using namespace BuildCon;

// These two defines just work around a bug in some compilers.
#define MERGE(x,y) MERGE2(x,y)
#define MERGE2(x,y) x ## y


#define BUILD_CONSTRAINT3(CT_NAME, function)  \
template<typename T1, typename T2, typename T3> \
Constraint*\
Build ## CT_NAME(StateObj* stateObj, const T1& t1, const T2& t2, const T3& t3, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { return reifyCon(stateObj,function(stateObj,t1,t2,t3), reifyVar); } \
  else \
  { return function(stateObj,t1,t2,t3); } \
}

#define BUILD_CONSTRAINT2(CT_NAME, function)  \
template<typename T1, typename T2> \
Constraint*\
Build ## CT_NAME(StateObj* stateObj, const T1& t1, const T2& t2, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { return reifyCon(stateObj, function(stateObj, t1,t2), reifyVar); } \
  else \
  { return function(stateObj,t1,t2); } \
}

#define BUILD_CONSTRAINT1(CT_NAME, function)  \
template<typename T1> \
Constraint*\
Build ## CT_NAME(StateObj* stateObj, const T1& t1, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { return reifyCon(stateObj, function(stateObj, t1), reifyVar); } \
  else \
  { return function(stateObj, t1); } \
}

#define BUILD_DYNAMIC_CONSTRAINT3(CT_NAME, function)  \
template<typename T1, typename T2, typename T3> \
DynamicConstraint*\
Build ## CT_NAME(StateObj* stateObj, const T1& t1, const T2& t2, const T3& t3, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { \
    cerr << "Cannot reify 'watched literal' constraints. Sorry." << endl; \
    exit(0); \
  } \
  else \
  { return function(stateObj,t1,t2,t3); } \
}

#define BUILD_DYNAMIC_CONSTRAINT2(CT_NAME, function)  \
template<typename T1, typename T2> \
DynamicConstraint*\
Build ## CT_NAME(StateObj* stateObj, const T1& t1, const T2& t2, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { \
    cerr << "Cannot reify 'watched literal' constraints. Sorry." << endl; \
    exit(0); \
  } \
  else \
  { return function(stateObj,t1,t2); } \
}

#define BUILD_DYNAMIC_CONSTRAINT1(CT_NAME, function)  \
template<typename T1> \
DynamicConstraint*\
Build ## CT_NAME(StateObj* stateObj,const T1& t1, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { \
    cerr << "Cannot reify 'watched literal' constraints. Sorry." << endl; \
    exit(0); \
  } \
  else \
  { return function(stateObj, t1); } \
}

#define TERMINATE_BUILDCON3(CT_NAME, TYPE) \
namespace BuildCon \
{ \
template<> \
struct Build ## TYPE<CT_NAME, 0> \
{ \
  template<typename T1, typename T2, typename T3> \
  static  \
  TYPE* build(StateObj* stateObj, const pair<pair<pair<EmptyType, light_vector<T1>* >, light_vector<T2>* >, light_vector<T3>*>& vars, ConstraintBlob& b, int) \
  { \
	if(b.implied_reified) \
	{ \
	  BoolVarRef reifyVar = getVars(stateObj).getBooleanContainer().get_var_num(b.reify_var.pos); \
	  BoolVarRef dummyVar; \
	  return truereifyCon(stateObj, Build ## CT_NAME(stateObj, *(vars.first.first.second), *(vars.first.second), *(vars.second), false, dummyVar, b), reifyVar); \
	} \
	else \
	{ \
	  BoolVarRef reifyVar; \
	  if(b.reified) \
	    reifyVar = getVars(stateObj).getBooleanContainer().get_var_num(b.reify_var.pos); \
	  return Build ## CT_NAME(stateObj, *(vars.first.first.second), *(vars.first.second), *(vars.second),  b.reified, reifyVar, b); \
	} \
  } \
}; \
} \

#define TERMINATE_BUILDCON2(CT_NAME, TYPE) \
namespace BuildCon \
{ \
template<> \
struct Build ## TYPE<CT_NAME, 0> \
{ \
  template<typename T1, typename T2> \
  static  \
  TYPE* build(StateObj* stateObj, const pair<pair<EmptyType, light_vector<T1>* >, light_vector<T2>* >& vars, ConstraintBlob& b, int) \
  { \
	if(b.implied_reified) \
	{ \
	  BoolVarRef reifyVar = getVars(stateObj).getBooleanContainer().get_var_num(b.reify_var.pos); \
	  BoolVarRef dummyVar; \
	  return truereifyCon(stateObj, Build ## CT_NAME(stateObj, *(vars.first.second), *(vars.second), false, dummyVar, b), reifyVar); \
	} \
	else \
	{ \
	  BoolVarRef reifyVar; \
	  if(b.reified) \
	    reifyVar = getVars(stateObj).getBooleanContainer().get_var_num(b.reify_var.pos); \
	  return Build ## CT_NAME(stateObj, *(vars.first.second), *(vars.second), b.reified, reifyVar, b); \
	} \
  } \
}; \
} \

#define TERMINATE_BUILDCON1(CT_NAME, TYPE) \
namespace BuildCon \
{ \
template<> \
struct Build ## TYPE<CT_NAME, 0> \
{ \
  template<typename T1> \
  static  \
  TYPE* build(StateObj* stateObj, const pair<EmptyType, light_vector<T1>* >& vars, ConstraintBlob& b, int) \
  { \
	if(b.implied_reified) \
	{ \
	  BoolVarRef reifyVar = getVars(stateObj).getBooleanContainer().get_var_num(b.reify_var.pos); \
	  BoolVarRef dummyVar; \
      return truereifyCon(stateObj, Build ## CT_NAME(stateObj, *(vars.second), false, dummyVar, b), reifyVar); \
	} \
	else \
	{ \
	  BoolVarRef reifyVar; \
	  if(b.reified) \
	    reifyVar = getVars(stateObj).getBooleanContainer().get_var_num(b.reify_var.pos); \
	  return Build ## CT_NAME(stateObj, *(vars.second), b.reified, reifyVar, b); \
	} \
  } \
}; \
} \


#define START_BUILDCON(CT_NAME, COUNT, TYPE) \
TYPE* \
build_constraint_ ## CT_NAME(StateObj* stateObj, ConstraintBlob& b) \
{ return Build ## TYPE<CT_NAME, COUNT>::build(stateObj, EmptyType(), b, 0); }

#define START_BUILDCON_INITIAL_LIST(CT_NAME, COUNT, TYPE) \
TYPE* \
build_constraint_ ## CT_NAME (StateObj* stateObj, ConstraintBlob& b) \
  { \
	  const vector<Var>& vars = b.vars[0]; \
	  light_vector<int> weights(vars.size()); \
	  for(unsigned i = 0; i < vars.size(); ++i) \
		weights[i] = vars[i].pos; \
	  return Build ## TYPE <CT_NAME, COUNT-1>::build(stateObj, make_pair(EmptyType(), &weights), b, 1); \
  }


#define BUILD_STATIC_CT(CT_NAME,COUNT) \
START_BUILDCON(CT_NAME, COUNT, Constraint) \
MERGE(TERMINATE_BUILDCON, COUNT)(CT_NAME, Constraint)

#define BUILD_DYNAMIC_CT(CT_NAME,COUNT) \
START_BUILDCON(CT_NAME, COUNT, DynamicConstraint) \
MERGE(TERMINATE_BUILDCON, COUNT)(CT_NAME, DynamicConstraint)

#define BUILD_STATIC_CT_INITIAL_LIST(CT_NAME, COUNT) \
START_BUILDCON_INITIAL_LIST(CT_NAME, COUNT, Constraint) \
MERGE(TERMINATE_BUILDCON,COUNT)(CT_NAME, Constraint)

#define BUILD_DYNAMIC_CT_INITIAL_LIST(CT_NAME, COUNT) \
START_BUILDCON_INITIAL_LIST(CT_NAME, COUNT, DynamicConstraint) \
MERGE(TERMINATE_BUILDCON,COUNT)(CT_NAME, DynamicConstraint)

#define BUILD_DEF_STATIC_CT(CT_NAME) \
Constraint* build_constraint_ ## CT_NAME(StateObj* stateObj, ConstraintBlob&);

#define BUILD_DEF_DYNAMIC_CT(CT_NAME) \
DynamicConstraint* build_constraint_ ## CT_NAME(StateObj* stateObj, ConstraintBlob&);


