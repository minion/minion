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

#define WATCHEDLITERALS

#include "minion.h"
#include "test_functions.h"

int allowed_tuples[][4] = {
{0,0,1,0},
{0,1,1,1}};

int tuple_count = 2;

vector<vector<int> > get_tuples()
{
  vector<vector<int> > tuples;
  for(int i = 0; i < tuple_count; ++i)
  {
	vector<int> v;
	for(int j = 0; j < 4; ++j)
	  v.push_back(allowed_tuples[i][j]);
	tuples.push_back(v);
  }
  return tuples;
}

int main(void)
{
  Controller::find_all_solutions();
  vector<BoolVarRef> vars;
  for(int i=0;i<4;i++)
    vars.push_back(boolean_container.get_new_var());
  
  vector<vector<int> > tuples;
  
  DynamicConstraint* c = GACTableCon(vars, get_tuples());
  
  Controller::add_constraint(c);
  
  add_to_var_order(vars);

  Controller::solve();
  cout << Controller::solutions << endl;
  D_ASSERT(Controller::solutions == 2);
  Controller::finish();
}



