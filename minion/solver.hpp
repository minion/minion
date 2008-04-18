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

inline void SearchState::addConstraint(Constraint* c) 
{ 
  constraints.push_back(c); 
  vector<AnyVarRef>* vars = c->get_vars_singleton();
  size_t vars_s = vars->size();
  for(size_t i = 0; i < vars_s; i++) //note all constraints the var is involved in
    (*vars)[i].addConstraint(c);
}
  
#ifdef DYNAMICTRIGGERS
inline void SearchState::addDynamicConstraint(DynamicConstraint* c) 
{ 
  dynamic_constraints.push_back(c); 
  vector<AnyVarRef>* vars = c->get_vars_singleton();
  size_t vars_s = vars->size();
  for(size_t i = 0; i < vars_s; i++) //note all constraints the var is involved in
    (*vars)[i].addConstraint(c);
}
#endif

namespace Controller
{
/// Pushes the state of the whole world.
inline void world_push(StateObj* stateObj)
{
  D_INFO(0,DI_SOLVER,"World Push");
D_ASSERT(getQueue(stateObj).isQueuesEmpty());
  getMemory(stateObj).backTrack().world_push();
}

/// Pops the state of the whole world.
inline void world_pop(StateObj* stateObj)
{
  D_INFO(0,DI_SOLVER,"World Pop");
D_ASSERT(getQueue(stateObj).isQueuesEmpty());
  getMemory(stateObj).backTrack().world_pop();
  getVars(stateObj).getBigRangevarContainer().bms_array.undo();
}

inline void world_pop_all(StateObj* stateObj)
{
int depth = getMemory(stateObj).backTrack().current_depth();
for(; depth > 0; depth--)
  world_pop(stateObj); 
}

}
