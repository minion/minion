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

#include "system.h"

#include "system/time_keeping.h"

// XXX These could possibly be turned off, but it's possible it will require
// Some small amount of work to make them work.
#define FULL_DOMAIN_TRIGGERS
#define DYNAMICTRIGGERS

#ifdef WATCHEDLITERALS
#define DYNAMICTRIGGERS
#endif


#define VERSION "Minion Version 0.3.2"
#define REVISION "Subversion (svn) Revision Number $Revision$"
// above line will work but only gives revision of this file,
//  not the current global revision 




/// Time at which search starts.
//VARDEF(clock_t setup_time);
//extern int setup_time;


#include "constants.h"
#include "debug.h"

#include "solver.h"

#include "memory_management/backtrackable_memory.h"
#include "memory_management/nonbacktrack_memory.h"

#include "reversible_vals.h"

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
VARDEF_ASSIGN(BOOL dynamic_triggers_used, false);
#include "constraints/constraint_dynamic.h"
#endif 

namespace Controller
{
  /// Add a new list of triggers to the queue.
  inline void push_triggers(TriggerRange new_triggers);
  inline void push_special_trigger(Constraint* trigger);
#ifdef DYNAMICTRIGGERS
  inline void push_dynamic_triggers(DynamicTrigger* trigs);
#endif
}

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

#include "constraints/constraint_occurrance.h"
#include "constraints/constraint_min.h"
#include "constraints/constraint_lex.h"
#include "constraints/constraint_neq.h"
#include "constraints/constraint_sum.h"
#include "constraints/constraint_less.h"
#include "constraints/constraint_and.h"
#include "constraints/constraint_lightsum.h"
#include "constraints/constraint_fullsum.h"
#include "constraints/constraint_element.h"
#include "constraints/constraint_GACelement.h"
#include "constraints/constraint_equal.h"
#include "constraints/constraint_weightedboolsum.h"
#include "constraints/constraint_unaryequals.h"
#include "constraints/constraint_unaryneq.h"
#include "constraints/constraint_product.h"

#ifdef DYNAMICTRIGGERS
#ifdef TRIES
#include "constraints/constraint_GACtable_trie.h"
#else

#ifdef REGINLHOMME
#include "constraints/constraint_GACtable_reginlhomme.h"
#else
#ifdef NIGHTINGALE
#include "constraints/constraint_GACtable_nightingale.h"
#else
#include "constraints/constraint_GACtable.h"
#endif 
#endif
#endif

#include "dynamic_constraints/dynamic_sum.h"
#include "dynamic_constraints/dynamic_element.h"
#include "dynamic_constraints/dynamic_vecneq.h"
#include "dynamic_constraints/dynamic_literalwatch.h"
#endif

#include "constraints/function_defs.hpp"

#include "queue/standard_queue.h"
#include "search/standard_search.h"
#include "constraint_setup.h"

#include "test_functions.h"

#endif

