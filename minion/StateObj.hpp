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

#ifndef STATEOBJ_HPP
#define STATEOBJ_HPP

#include "StateObj_forward.h"

VARDEF(Memory searchMem_m);
VARDEF(SearchOptions options_m);
VARDEF(SearchState state_m);
VARDEF(Queues queues_m);
VARDEF(VariableContainer varContainer_m);
VARDEF(BoolContainer bools_m);

inline BoolContainer &getBools() { return bools_m; }
inline SearchOptions &getOptions() { return options_m; }
inline SearchState &getState() { return state_m; }
inline Queues &getQueue() { return queues_m; }
inline Memory &getMemory() { return searchMem_m; }
inline VariableContainer &getVars() { return varContainer_m; }

template <typename DomType>
inline BoundVarContainer<DomType> &BoundVarRef_internal<DomType>::getCon_Static() {
  return varContainer_m.boundVarContainer;
}

inline BoolVarContainer &BoolVarRef_internal::getCon_Static() {
  return varContainer_m.boolVarContainer;
}

template <typename DomType>
inline SparseBoundVarContainer<DomType> &SparseBoundVarRef_internal<DomType>::getCon_Static() {
  return varContainer_m.sparseBoundVarContainer;
}

template <typename d_type>
inline BigRangeVarContainer<d_type> &BigRangeVarRef_internal_template<d_type>::getCon_Static() {
  return varContainer_m.bigRangeVarContainer;
}

// Must be defined later.
inline SearchState::~SearchState() {
  for (SysInt i = 0; i < (SysInt)constraints.size(); ++i)
    delete constraints[i];
}

#endif
