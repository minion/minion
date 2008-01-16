/*
 *  BuildDynamicConstraint.cpp
 *  cutecsp
 *
 *  Created by Chris Jefferson on 14/04/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */



#define NO_MAIN

#include "minion.h"
#include "CSPSpec.h"

using namespace ProbSpec;

#ifdef DYNAMICTRIGGERS

#include "BuildConstraint.h"

namespace BuildCon
{
DynamicConstraint*
build_dynamic_constraint(ConstraintBlob& b)
{
  switch(b.constraint.type)
  {
    case CT_WATCHED_LEQSUM:
	case CT_WATCHED_GEQSUM:
	case CT_WATCHED_ELEMENT:
	case CT_WATCHED_VECNEQ:
	  return build_dynamic_constraint_binary(b);
	case CT_WATCHED_TABLE:
	  return build_dynamic_constraint_table(b);
	default:
	  D_FATAL_ERROR( "unsupported constraint");
  }
};
}
#endif


