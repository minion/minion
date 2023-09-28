// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

// This file just defines all global variables, using "IN_MAIN"

// These are just because VC++ sucks.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

#ifndef IN_MAIN
#define IN_MAIN
#endif

#include "minion.h"

#include "constraint_defs.h"

SysInt numOfConstraints = sizeof(constraint_list) / sizeof(ConstraintDef);
