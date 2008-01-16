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

#include "minion.h"
#include "test_functions.h"


int main(void)
{
  Controller::find_all_solutions();
  vector<BoolVarRef> vars;
  vector<BoolVarRef> var1(3);
  for(int i=0;i<4;i++)
    vars.push_back(boolean_container.get_new_var());
  
  for(int i=0;i<3;i++)
  {
    var1[i]=vars[i];
  }
  
  Constraint* c = BoolLessEqualSumCon(var1, compiletime_val<1>());
 
  Controller::add_constraint(reifyCon(c, vars[3]));
  
  add_to_var_order(vars);

  Controller::solve();
  D_ASSERT(Controller::solutions == 8);
  Controller::finish();
}



