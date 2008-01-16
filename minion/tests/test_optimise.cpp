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

int vals[] = {1,4,5,10,17,18,19};

  SparseBoundVarRef a,b;

void print_sol(void)
{ cout << a.getAssignedValue() << " " << b.getAssignedValue() << endl; }

int main(void)
{
  //Controller::solution_trigger = &print_sol;
  vector<unsigned char> v(7);
  copy(vals,vals + 7, v.begin());
  a = sparse_boundvar_container.get_new_var(v);
  b = sparse_boundvar_container.get_new_var(v);
  vector<SparseBoundVarRef> vars;
  vars.push_back(a);
  vars.push_back(b);
  Controller::optimise_maximise_var(a);
  add_to_var_order(vars);
  Controller::solve();
  D_ASSERT(Controller::solutions == 7);
}
