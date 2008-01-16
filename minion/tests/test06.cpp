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
  vector<BoolVarRef> vars;
  vector<BoolVarRef> var1(3);
  vector<BoolVarRef> var2(3);
  for(int i=0;i<6;i++)
    vars.push_back(boolean_container.get_new_var());
  
  for(int i=0;i<3;i++)
  {
    var1[i]=vars[i];
    var2[i]=vars[i+3];
  }
  
  Constraint* c = AndCon(vars[0], vars[1], vars[2]);
  Constraint* e = c->get_table_constraint();
  Constraint* d = AndCon(vars[3], vars[4], vars[5]);
  var1.push_back(vars[2]);
  var2.push_back(vars[5]);
  Controller::lock();
  test_equal(var1,var2);
}

