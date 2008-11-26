/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

// This file just defines all global variables, using "IN_MAIN"

// These are just because VC++ sucks.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

#ifndef IN_MAIN
#define IN_MAIN
#endif

#include "minion.h"
#include "CSPSpec.h"

#include "system/defined_macros.h"

#include "build_constraints/constraint_defs.h"

int num_of_constraints = sizeof(constraint_list) / sizeof(ConstraintDef);
