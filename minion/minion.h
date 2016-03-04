/*
 * Minion http://minion.sourceforge.net
 * Copyright (C) 2006-09
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */
#ifndef MINION_H
#define MINION_H

#include "system/system.h"
#include "BuildDefines.h"

// These are just because VC++ sucks.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

static const char MinionVersion[] = "Minion Version 2";

// above line will work but only gives revision of this file,
//  not the current global revision
#include "get_info/get_info.h"
#include "solver.h"
VARDEF(ofstream solsoutFile);

#include "memory_management/backtrackable_memory.h"

#include "memory_management/trailed_monotonic_set_new.h"
#include "memory_management/nonbacktrack_memory.h"
//#include "memory_management/trailed_monotonic_set.hpp"
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
//#include "../constraints/constraint_checkassign.h"

#include "lock.h"
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
