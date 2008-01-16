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

#include "minion.h"

/*
void print_sol(void)
{
  for(int i=0;i<height;++i)
  {
    for(int j=0;j<width;++j)
    {
      D_ASSERT(vars[i][j].isAssigned());
      printf("%d",vars[i][j].getAssignedValue());
    }
    printf("\n");
  }
  printf("\n");
}
*/

int main (int argc, char * const argv[]) {
  Controller::find_all_solutions();
  vector<BoolVarRef> vars;
  vector<int> vals;
  // set_solution_function(&print_sol);
  for(int i =  0; i < 3; i++)
  {
    vars.push_back(boolean_container.get_new_var());
    vals.push_back(i + 1);
  }
  ConstantVar convar(3);
  Controller::add_constraint(LeqWeightBoolSumCon(vars,vals,convar));
  Controller::lock();
  Controller::propogate_queue();
  D_ASSERT(!Controller::failed);
  D_ASSERT(!vars[0].isAssigned());
  D_ASSERT(!vars[1].isAssigned());
  D_ASSERT(!vars[2].isAssigned());
 
  
  Controller::finish();
}
