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
// This header is designed to be included after all other headers



VARDEF_ASSIGN(clock_t time_limit, 0);

VARDEF(unsigned long long nodes);


namespace Controller
{

  VARDEF_ASSIGN(AnyVarRef* optimise_var, NULL);
  VARDEF(int current_optimise_position);
  VARDEF_ASSIGN(BOOL optimise, false);

  VARDEF(vector<Constraint*> constraints);
#ifdef DYNAMICTRIGGERS
  VARDEF(vector<DynamicConstraint*> dynamic_constraints);
#endif
  VARDEF(int solutions);
  VARDEF(vector<vector<AnyVarRef> > print_matrix);
  VARDEF_ASSIGN(BOOL print_only_solution, false);
  VARDEF_ASSIGN(BOOL commandlineoption_dumptree, false);
  VARDEF_ASSIGN(int commandlineoption_sollimit, -1);
  VARDEF_ASSIGN(BOOL commandlineoption_fullpropogate, false);
  VARDEF_ASSIGN(BOOL commandlineoption_nocheck, false);
  VARDEF_ASSIGN(unsigned long long commandlineoption_nodelimit, 0);
  
  
  /// Pushes the state of the whole world.
  inline void world_push()
  {
    D_INFO(0,DI_SOLVER,"World Push");
    backtrackable_memory.world_push();
  }
  
  /// Pops the state of the whole world.
  inline void world_pop()
  {
    D_INFO(0,DI_SOLVER,"World Pop");
    backtrackable_memory.world_pop();
    big_rangevar_container.bms_array->undo();
  }
  
  inline void world_pop_all()
  {
	int depth = backtrackable_memory.current_depth();
	for(int i = 0; i < depth; ++i)
	  world_pop(); 
  }
}

