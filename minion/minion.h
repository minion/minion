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

#include "svn_header.h"
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


#define VERSION "Minion Version 0.4.1"
#define REVISION "Subversion (svn) Revision Number $Revision$"
// above line will work but only gives revision of this file,
//  not the current global revision 



#ifdef MORE_SEARCH_INFO
#include "get_info/get_info.h"
#else
#define CON_INFO_ADDONE(ConEvent)
#define VAR_INFO_ADDONE(VarType, VarEvent)
#define PROP_INFO_ADDONE(PropType)
#endif

#include "solver.h"

VARDEF(TableOut tableout);

#include "memory_management/backtrackable_memory.h"
#include "memory_management/nonbacktrack_memory.h"

#include "reversible_vals.h"

#include "memory_management/trailed_monotonic_set.h"

typedef TrailedMonotonicSet MonotonicSet;

#include "tuple_container.h"

/** @brief Represents a change in domain. 
 *
 * This is used instead of a simple int as the use of various mappers on variables might mean the domain change needs
 * to be corrected. Every variable should implement the function getDomainChange which uses this and corrects the domain.
 */
class DomainDelta
{ 
  int domain_change; 
public:
  /// This function shouldn't be called directly. This object should be passed to a variables, which will do any "massaging" which 
  /// is required.
  int XXX_get_domain_diff()
{ return domain_change; }

  DomainDelta(int i) : domain_change(i)
{}
};


struct Trigger;
struct Constraint;
struct DynamicTrigger;

#include "constraints/triggers.h"

#include "variables/VarRefType.h"
#include "variables/AnyVarRef.h"
#include "constraints/constraint.h"

#ifdef DYNAMICTRIGGERS
#include "constraints/constraint_dynamic.h"
#endif 

/*
namespace Controller
{
  /// Add a new list of triggers to the queue.
  inline void queues->pushTriggers(TriggerRange new_triggers);
  inline void push_special_trigger(Constraint* trigger);
#ifdef DYNAMICTRIGGERS
  inline void push_dynamic_triggers(DynamicTrigger* trigs);
#endif
}
*/

#include "queue/standard_queue.h"

#include "trigger_list.h"

#include "variables/variables.h"

#include "solver2.h"

#include "build_constraints/build_helper.h"

// This constraint must be listed early so that it can
// be called by all constraints.
#include "constraints/constraint_checkassign.h"

#include "constraints/constraint_table.h"
#include "constraints/constraint_boundtable.h"


#include "constraints/reify.h"
#include "constraints/reify_true.h"

#include "constraints/function_defs.hpp"



#include "search/standard_search.h"
#include "search/recursive_search.h"

#include "search/search_control.h"

#include "constraint_setup.h"

#include "preprocess/preprocess.h"

#include "test_functions.h"

#endif
