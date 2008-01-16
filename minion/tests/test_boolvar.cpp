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

const int varcount = 400;

int main(void)
{
  vector<BoolVarRef> vars(varcount);
  for(int i=0;i<varcount;i++)
    vars[i] = boolean_container.get_new_var();
Controller::lock();
Controller::propogate_queue();
  for(int i = 0; i < varcount; ++i)
  {
    for(int j = 0; j < i; ++j)
    { D_ASSERT(vars[j].isAssigned()); }
    for(int j = i; j < varcount; ++j)
    { D_ASSERT(!vars[j].isAssigned()); }
    vars[i].propogateAssign(1);
  }
Controller::finish();
}
