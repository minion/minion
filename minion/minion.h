/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/* Minion
* Copyright (C) 2006
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef MINION_H
#define MINION_H

#ifdef REENTER
#define MANY_VAR_CONTAINERS
#endif

#include "system/system.h"
#include "constants.h"

#ifdef USELIGHTVECTOR
#include "system/light_vector.h"
#else
#define light_vector vector
#endif

// XXX These could possibly be turned off, but it's possible it will require
// Some small amount of work to make them work.
#define FULL_DOMAIN_TRIGGERS
#define DYNAMICTRIGGERS

#ifdef WATCHEDLITERALS
#define DYNAMICTRIGGERS
#endif

#define VERSION "Minion Version 0.5.1"
#define REVISION "Subversion (svn) Revision Number $Revision$"
// above line will work but only gives revision of this file,
//  not the current global revision 

#include "get_info/get_info.h"

#include "solver.h"

VARDEF(TableOut tableout);
VARDEF(ofstream solsoutFile);

#include "memory_management/backtrackable_memory.h"
#include "memory_management/nonbacktrack_memory.h"
#include "reversible_vals.h"
#include "memory_management/trailed_monotonic_set.h"

typedef TrailedMonotonicSet MonotonicSet;

#include "constraints/constraint.h"

#ifdef DYNAMICTRIGGERS
#include "constraints/constraint_dynamic.h"
#endif 

#include "queue/standard_queue.h"

#include "trigger_list.h"

#include "variables/variables.h"

// TODO: Move this!
namespace Controller
{
  VARDEF(vector<vector<AnyVarRef> > print_matrix);  
}

#include "build_constraints/build_helper.h"

// This constraint must be listed early so that it can
// be called by all constraints.
#include "constraints/constraint_checkassign.h"

#include "constraints/reify.h"
#include "constraints/reify_true.h"

#include "preprocess.h"
#include "constraint_setup.h"
#include "test_functions.h"
#include "BuildCSP.h"

#include "StateObj.hpp"
#include "solver.hpp"
#include "constraints/function_defs.hpp"
#include "constraints/constraint_abstract.hpp"

#endif
