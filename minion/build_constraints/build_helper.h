/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

#define NO_MAIN

//#include "../minion.h"
#include "../CSPSpec.h"


#include "../BuildConstraintConstructs.h"

using namespace BuildCon;

// These two defines just work around a bug in some compilers.
#define MERGE(x,y) MERGE2(x,y)
#define MERGE2(x,y) x ## y


#define BUILD_CONSTRAINT3(CT_NAME, function)  \
template<typename T1, typename T2, typename T3> \
AbstractConstraint*\
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
AbstractConstraint*\
Build ## CT_NAME(StateObj* stateObj, const T1& t1, const T2& t2, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { return reifyCon(stateObj, function(stateObj, t1,t2), reifyVar); } \
  else \
  { return function(stateObj,t1,t2); } \
}

#define BUILD_CONSTRAINT2_WITH_BLOB(CT_NAME, function)  \
template<typename T1, typename T2> \
AbstractConstraint*\
Build ## CT_NAME(StateObj* stateObj, const T1& t1, const T2& t2, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { return reifyCon(stateObj, function(stateObj, t1,t2, b), reifyVar); } \
  else \
  { return function(stateObj,t1,t2, b); } \
}

#define BUILD_CONSTRAINT1(CT_NAME, function)  \
template<typename T1> \
AbstractConstraint*\
Build ## CT_NAME(StateObj* stateObj, const T1& t1, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { return reifyCon(stateObj, function(stateObj, t1), reifyVar); } \
  else \
  { return function(stateObj, t1); } \
}

#define BUILD_CONSTRAINT1_WITH_BLOB(CT_NAME, function)  \
template<typename T1> \
AbstractConstraint*\
Build ## CT_NAME(StateObj* stateObj, const T1& t1, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { return reifyCon(stateObj, function(stateObj, t1, b), reifyVar); } \
  else \
  { return function(stateObj, t1, b); } \
}

// This one has to be inline because it's not templated, and we want to avoid multiple definitions if it is
// included multiple times

#define BUILD_CONSTRAINT0(CT_NAME, function)  \
inline AbstractConstraint*\
Build ## CT_NAME(StateObj* stateObj, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { return reifyCon(stateObj, function(stateObj), reifyVar); } \
  else \
  { return function(stateObj); } \
}

#define BUILD_DYNAMIC_CONSTRAINT3(CT_NAME, function)  \
template<typename T1, typename T2, typename T3> \
AbstractConstraint*\
Build ## CT_NAME(StateObj* stateObj, const T1& t1, const T2& t2, const T3& t3, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { \
    FAIL_EXIT("Cannot reify 'watched literal' constraints. Sorry."); \
  } \
  else \
  { return function(stateObj,t1,t2,t3); } \
}

#define BUILD_DYNAMIC_CONSTRAINT2(CT_NAME, function)  \
template<typename T1, typename T2> \
AbstractConstraint*\
Build ## CT_NAME(StateObj* stateObj, const T1& t1, const T2& t2, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { \
    FAIL_EXIT("Cannot reify 'watched literal' constraints. Sorry."); \
  } \
  else \
  { return function(stateObj,t1,t2); } \
}

#define BUILD_DYNAMIC_CONSTRAINT1(CT_NAME, function)  \
template<typename T1> \
AbstractConstraint*\
Build ## CT_NAME(StateObj* stateObj,const T1& t1, bool reify, \
				 const BoolVarRef& reifyVar, ConstraintBlob& b) \
{ \
  if(reify) \
  { \
    FAIL_EXIT("Cannot reify 'watched literal' constraints. Sorry.") \
  } \
  else \
  { return function(stateObj, t1); } \
}

#define TERMINATE_BUILDCON3(CT_NAME) \
namespace BuildCon \
{ \
template<> \
struct BuildConObj<CT_NAME, 0> \
{ \
  template<typename T1, typename T2, typename T3> \
  static  \
  AbstractConstraint* build(StateObj* stateObj, const pair<pair<pair<EmptyType, light_vector<T1>* >, light_vector<T2>* >, light_vector<T3>*>& vars, ConstraintBlob& b, int) \
  { \
	  BoolVarRef reifyVar; \
	  return Build ## CT_NAME(stateObj, *(vars.first.first.second), *(vars.first.second), *(vars.second), false, reifyVar, b); \
  } \
}; \
} \

#define TERMINATE_BUILDCON2(CT_NAME) \
namespace BuildCon \
{ \
template<> \
struct BuildConObj<CT_NAME, 0> \
{ \
  template<typename T1, typename T2> \
  static  \
  AbstractConstraint* build(StateObj* stateObj, const pair<pair<EmptyType, light_vector<T1>* >, light_vector<T2>* >& vars, ConstraintBlob& b, int) \
  { \
	  BoolVarRef reifyVar; \
	  return Build ## CT_NAME(stateObj, *(vars.first.second), *(vars.second), false, reifyVar, b); \
  } \
}; \
} \

#define TERMINATE_BUILDCON1(CT_NAME) \
namespace BuildCon \
{ \
template<> \
struct BuildConObj<CT_NAME, 0> \
{ \
  template<typename T1> \
  static  \
  AbstractConstraint* build(StateObj* stateObj, const pair<EmptyType, light_vector<T1>* >& vars, ConstraintBlob& b, int) \
  { \
	  BoolVarRef reifyVar; \
	  return Build ## CT_NAME(stateObj, *(vars.second), false, reifyVar, b); \
  } \
}; \
} \

#define TERMINATE_BUILDCON0(CT_NAME) \
namespace BuildCon \
{ \
template<> \
struct BuildConObj<CT_NAME, 0> \
{ \
  static  \
  AbstractConstraint* build(StateObj* stateObj, const EmptyType& vars, ConstraintBlob& b, int) \
  { \
	  BoolVarRef reifyVar; \
	  return Build ## CT_NAME(stateObj, false, reifyVar, b); \
  } \
}; \
} \

#define START_BUILDCON(CT_NAME, COUNT) \
AbstractConstraint* \
build_constraint_ ## CT_NAME(StateObj* stateObj, ConstraintBlob& b) \
{ return BuildConObj<CT_NAME, COUNT>::build(stateObj, EmptyType(), b, 0); }

#define BUILD_STATIC_CT(CT_NAME,COUNT) \
MERGE(TERMINATE_BUILDCON, COUNT)(CT_NAME) \
START_BUILDCON(CT_NAME, COUNT)

#define BUILD_DYNAMIC_CT(CT_NAME,COUNT) \
MERGE(TERMINATE_BUILDCON, COUNT)(CT_NAME) \
START_BUILDCON(CT_NAME, COUNT)


#define BUILD_DEF_STATIC_CT(CT_NAME) \
AbstractConstraint* build_constraint_ ## CT_NAME(StateObj* stateObj, ConstraintBlob&);

#define BUILD_DEF_DYNAMIC_CT(CT_NAME) \
AbstractConstraint* build_constraint_ ## CT_NAME(StateObj* stateObj, ConstraintBlob&);


