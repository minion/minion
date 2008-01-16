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

const int size = 3;

int main(void)
{
  vector<LRangeVarRef> vars;
  vector<LRangeVarRef> x1(size);
  vector<LRangeVarRef> y1(size);
  vector<LRangeVarRef> x2(size);
  vector<LRangeVarRef> y2(size);
  
  
  for(int i=0;i<16;i++)
    vars.push_back(rangevar_container.get_new_var(0,size));
  
  for(int i=0;i<size;i++)
  {
    x1[i]=vars[i];
    y1[i]=vars[i + size];
    x2[i]=vars[i + size*2];
    y2[i]=vars[i + size*3];
  }
  
  Constraint* c = LexLeqCon(x1, y1);
  Constraint* d = LexLeqCon(x2, y2);
  for(int i = 0;i < size;i++)
  {
    x1.push_back(y1[i]);
    x2.push_back(y2[i]);
  }

  Constraint* e = c->get_table_constraint();
  Controller::add_constraint(d);
  Controller::add_constraint(e);
  Controller::lock();
  test_equal(x1, x2);
}





