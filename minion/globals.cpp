// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

// This file just defines all global variables, using "IN_MAIN"

// These are just because VC++ sucks.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

#ifndef IN_MAIN
#define IN_MAIN
#endif

#include "globals.h"
#include "minion.h"

#include "constraint_defs.h"

#ifdef LIBMINION
Globals* globals = new Globals();
#endif

SysInt numOfConstraints = sizeof(constraint_list) / sizeof(ConstraintDef);

Globals::Globals() {
    searchMem_m = NULL;
    options_m = NULL;
    state_m = NULL;
    queues_m = NULL;
    varContainer_m = NULL;
    bools_m = NULL;
    parData_m = NULL;
}

Globals::~Globals() {
  delete this->bools_m;
  delete this->state_m;
  delete this->queues_m;
  delete this->options_m;
  // No parallel minion in library usage for now
  //delete this->parData_m; //FIXME: having this in causes runMinion to segfault!
  delete this->varContainer_m;
}
