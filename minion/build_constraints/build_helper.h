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
	  return Build ## CT_NAME(stateObj, *(vars.first.first.second), *(vars.first.second), *(vars.second), b); \
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
	  return Build ## CT_NAME(stateObj, *(vars.first.second), *(vars.second), b); \
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
	  return Build ## CT_NAME(stateObj, *(vars.second), b); \
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
	  return Build ## CT_NAME(stateObj, b); \
  } \
}; \
} \

#define START_BUILDCON(CT_NAME, COUNT) \
AbstractConstraint* \
build_constraint_ ## CT_NAME(StateObj* stateObj, ConstraintBlob& b) \
{ return BuildConObj<CT_NAME, COUNT>::build(stateObj, EmptyType(), b, 0); }

#define BUILD_CT(CT_NAME,COUNT) \
MERGE(TERMINATE_BUILDCON, COUNT)(CT_NAME) \
START_BUILDCON(CT_NAME, COUNT)

#define BUILD_DEF_CT(CT_NAME) \
AbstractConstraint* build_constraint_ ## CT_NAME(StateObj* stateObj, ConstraintBlob&);


