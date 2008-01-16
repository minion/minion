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

vector<BoolVarRef> vars;


int main(int argc, char** argv)
{
  int shuffle = 0;
  if(argc > 1)
    shuffle = atoi(argv[1]);
  
  
  
  vector<BoolVarRef> x1(4);
  vector<BoolVarRef> y1(4);
  
  
  for(int i=0;i<9;i++)
    vars.push_back(boolean_container.get_new_var());
  
  for(int i=0;i<4;i++)
  {
    x1[i] = vars[i];
    y1[i] = vars[i + 4];
  }
  
  
  Constraint* c = LexLeqCon(x1, y1);
  Controller::add_constraint(reifyCon(c, vars[8]));
  
  
 
  for(int i=0;i<shuffle;i++)
    random_shuffle(vars.begin(), vars.end());
  add_to_var_order(vars);
  Controller::solve();
  D_ASSERT(Controller::solutions == 256);
  
}
