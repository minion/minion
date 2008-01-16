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
  vector<BigRangeVarRef> vars;
  vector<BigRangeVarRef> var1(3);
  vector<BigRangeVarRef> var2(3);
  for(int i=0;i<10;i++)
    vars.push_back(big_rangevar_container.get_new_var(-2,4));
  
  for(int i=0;i<3;i++)
  {
    var1[i]=vars[i];
    var2[i]=vars[i+5];
  }
  
  Constraint* c = GACElementCon(var1, vars[3], vars[4]);
  Constraint* d = GACElementCon(var2, vars[8], vars[9]);
  Constraint* e = c->get_table_constraint();
  Controller::add_constraint(d);
  Controller::add_constraint(e);
  Controller::lock();
  test_equal(var1,var2,true);
}



