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
#define VAR_COUNT 5
#define VAR_SUM 1



int main(void)
{
  D_ASSERT(ipow(5,1)==5);
  D_ASSERT(ipow(5,3)==125);
  vector<BoolVarRef> vars;
  vector<BoolVarRef> var1(VAR_COUNT);
  vector<BoolVarRef> var2(VAR_COUNT);
  for(int i=0;i<VAR_COUNT*2;i++)
    vars.push_back(boolean_container.get_new_var());
  
  for(int i=0;i<VAR_COUNT;i++)
  {
    var1[i]=vars[i];
    var2[i]=vars[i+VAR_COUNT];
  }
  
  Constraint* c = BoolLessEqualSumCon(var1, compiletime_val<VAR_SUM>());
  Constraint* d = BoolLessEqualSumCon(var2, compiletime_val<VAR_SUM>());
  Constraint* e = c->get_table_constraint();
  Controller::add_constraint(d);
  Controller::add_constraint(e);
  Controller::lock();
  test_equal(var1,var2);
  }

