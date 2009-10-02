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

#ifndef STATEOBJ_FORWARD
#define STATEOBJ_FORWARD

// The following is a little trick, to make sure no-one accidentally links together
// debugging and non-debugging code (which are not link-compatable)
#ifdef MINION_DEBUG
namespace StateObjNamespace_DEBUG
#else
namespace StateObjNamespace_RELEASE
#endif
{ struct StateObj; }
 
#ifdef MINION_DEBUG
using namespace StateObjNamespace_DEBUG;
#else
using namespace StateObjNamespace_RELEASE;
#endif

class BoolContainer;
class SearchOptions;
class SearchState;
class Queues;
class Memory;
class TriggerMem;
class VariableContainer;


inline BoolContainer& getBools(StateObj* stateObj);
inline SearchOptions& getOptions(StateObj* stateObj);
inline SearchState& getState(StateObj* stateObj);
inline Queues& getQueue(StateObj* stateObj);
inline Memory& getMemory(StateObj* stateObj);
inline TriggerMem& getTriggerMem(StateObj* stateObj);
inline VariableContainer& getVars(StateObj* stateObj);


#endif