// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0
#ifndef MINION_H
#define MINION_H

#include "BuildDefines.h"
#include "system/system.h"

#include "globals.h"

// These are just because VC++ sucks.
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE 1
#endif

static const char MinionVersion[] = "Minion Version 2";

// above line will work but only gives revision of this file,
//  not the current global revision
#include "get_info/get_info.h"
#include "solver.h"
VARDEF(ofstream solsoutfile);

#include "memory_management/MemoryBlock.h"

#include "memory_management/trailed_monotonic_set_new.h"

#include "memory_management/nonbacktrack_memory.h"
// #include "memory_management/trailed_monotonic_set.hpp"
#include "memory_management/monotonic_set_wrapper.h"
#include "memory_management/reversible_vals.h"

typedef TrailedMonotonicSet MonotonicSet;

#include "triggering/constraint_abstract.h"

#include "queue/standard_queue.h"

#include "triggering/trigger_list.h"

#include "variables/variables.h"

#ifndef DOMINION
#include "build_constraints/build_helper.h"
#endif

// This constraint must be listed early so that it can
// be called by all constraints.
// #include "../constraints/constraint_checkassign.h"

#ifndef DOMINION
#include "BuildCSP.h"
#endif

#include "StateObj.hpp"
#include "solver.hpp"

#ifndef DOMINION
#include "inputfile_parse/CSPSpec.hpp"
#else
inline void inputPrint(std::ostream&, const Var&) {}
#endif

#include "triggering/dynamic_trigger.hpp"
#endif
