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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef STATEOBJ_HPP
#define STATEOBJ_HPP

#include "StateObj_forward.h"

// The following is a little trick, to make sure no-one accidentally links together
// debugging and non-debugging code (which are not link-compatable)

#ifdef MINION_DEBUG
namespace StateObjNamespace_DEBUG
#else
namespace StateObjNamespace_RELEASE
#endif
{ struct StateObj {}; }

#ifdef MINION_DEBUG
using namespace StateObjNamespace_DEBUG;
#else
using namespace StateObjNamespace_RELEASE;
#endif

VARDEF(StateObj _noreenter_stateObj);
VARDEF(Memory searchMem_m);
VARDEF(SearchOptions options_m);
VARDEF_ASSIGN(SearchState state_m, &_noreenter_stateObj);
VARDEF_ASSIGN(Queues queues_m, &_noreenter_stateObj);
VARDEF_ASSIGN(TriggerMem triggerMem_m, &_noreenter_stateObj);
VARDEF_ASSIGN(VariableContainer varContainer_m, &_noreenter_stateObj);
VARDEF_ASSIGN(BoolContainer bools_m, &_noreenter_stateObj);

inline BoolContainer& getBools(StateObj* stateObj)
{ return bools_m; }
inline SearchOptions& getOptions(StateObj* stateObj)
{ return options_m; }
inline SearchState& getState(StateObj* stateObj)
{ return state_m; }
inline Queues& getQueue(StateObj* stateObj)
{ return queues_m; }
inline Memory& getMemory(StateObj* stateObj)
{ return searchMem_m; }
inline TriggerMem& getTriggerMem(StateObj* stateObj)
{ return triggerMem_m; }
inline VariableContainer& getVars(StateObj* stateObj)
{ return varContainer_m; }

template<typename DomType>
inline BoundVarContainer<DomType>& BoundVarRef_internal<DomType>::getCon_Static()
{ return varContainer_m.boundVarContainer; }

inline BoolVarContainer& BoolVarRef_internal::getCon_Static()
{ return varContainer_m.boolVarContainer; }

template<typename DomType>
inline SparseBoundVarContainer<DomType>& SparseBoundVarRef_internal<DomType>::getCon_Static()
{ return varContainer_m.sparseBoundVarContainer; }

template<typename d_type>
inline BigRangeVarContainer<d_type>& BigRangeVarRef_internal_template<d_type>::getCon_Static()
{ return varContainer_m.bigRangeVarContainer; }

// Must be defined later.
inline SearchState::~SearchState()
{
  for(SysInt i = 0; i < (SysInt)constraints.size(); ++i)
    delete constraints[i];
}


#endif
