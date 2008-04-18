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


#ifndef STATEOBJ_HPP
#define STATEOBJ_HPP

#ifdef REENTER
struct StateObj
{
  // Forbid copying this type!
  StateObj(const StateObj&);
  void operator=(const StateObj&);
  
  Memory* searchMem_m;
  SearchOptions* options_m;
  SearchState state_m;
  Queues* queues_m;
  TriggerMem* triggerMem_m;
  VariableContainer* varContainer_m;
  BoolContainer* backtrack_bools; 
public:
  
 
  
  StateObj() :
    searchMem_m(new Memory),
    options_m(new SearchOptions),
    state_m(),
    queues_m(new Queues(this)),
    triggerMem_m(new TriggerMem(this)),
    varContainer_m(new VariableContainer(this)),
    backtrack_bools(new BoolContainer(this))
  { }

  ~StateObj()
  { 
    delete backtrack_bools;
    delete varContainer_m;
    delete triggerMem_m;
    delete queues_m;
    delete options_m;
    delete searchMem_m;    
  }
};

inline BoolContainer& getBools(StateObj* stateObj)
{ return *(stateObj->backtrack_bools); }
inline SearchOptions& getOptions(StateObj* stateObj)
{ return *(stateObj->options_m); }
inline SearchState& getState(StateObj* stateObj)
{ return stateObj->state_m; }
inline Queues& getQueue(StateObj* stateObj)
{ return *(stateObj->queues_m); }
inline Memory& getMemory(StateObj* stateObj)
{ return *(stateObj->searchMem_m); }
inline TriggerMem& getTriggerMem(StateObj* stateObj)
{ return *(stateObj->triggerMem_m); }
inline VariableContainer& getVars(StateObj* stateObj)
{ return *(stateObj->varContainer_m); }

#else

struct StateObj {};
VARDEF(StateObj _noreenter_stateObj);
VARDEF(Memory searchMem_m);
VARDEF(SearchOptions options_m);
VARDEF(SearchState state_m);
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
#endif

#ifndef MANY_VAR_CONTAINERS
template<typename DomType>
inline BoundVarContainer<DomType>& BoundVarRef_internal<DomType>::getCon_Static()
{ return varContainer_m.boundvarContainer; }

inline BooleanContainer& BoolVarRef_internal::getCon_Static()
{ return varContainer_m.booleanContainer; }

template<typename DomType>
inline SparseBoundVarContainer<DomType>& SparseBoundVarRef_internal<DomType>::getCon_Static()
{ return varContainer_m.sparseBoundvarContainer; }

template<typename d_type>
inline BigRangeVarContainer<d_type>& BigRangeVarRef_internal_template<d_type>::getCon_Static()
{ return varContainer_m.bigRangevarContainer; }

#endif

// Must be defined later.
inline SearchState::~SearchState()
{ 
  for(int i = 0; i < constraints.size(); ++i)
    delete constraints[i];
#ifdef DYNAMICTRIGGERS
  for(int i = 0; i < dynamic_constraints.size(); ++i)
    delete dynamic_constraints[i];
#endif
}

#endif
