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

#ifndef WATCHEDLITERALS
#define WATCHEDLITERALS
#endif

#include "minion.h"
#include "test_functions.h"


int main(void)
{
  vector<LRangeVarRef> vars;
  vector<LRangeVarRef> var1(4);
  vector<LRangeVarRef> var2(4);
  for(int i=0;i<12;i++)
    vars.push_back(rangevar_container.get_new_var(0,5));
  
  for(int i=0;i<4;i++)
  {
    var1[i]=vars[i];
    var2[i]=vars[i+6];
  }
  
  Constraint* c = ElementCon(var1, vars[4], vars[5]);
  DynamicConstraint* d = ElementConDynamic(var2, vars[10], vars[11]);
  Constraint* e = c->get_table_constraint();
  Controller::add_constraint(d);
  Controller::add_constraint(e);
  Controller::lock();
  test_equal(var1, var2, true);
}


