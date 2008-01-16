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

int vals[] = {1,4,5,10,17,18,19};

int main(void)
{
  SparseBoundVarRef a;
  vector<short> v(7);
  copy(vals,vals + 7, v.begin());
  a = sparse_boundvar_container.get_new_var(v);
  Controller::add_constraint(TestCon(a));
  Controller::lock();
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  D_ASSERT(a.getMin() == 1);
  D_ASSERT(a.getMax() == 19);
  
  Controller::world_push();
  a.setMin(4);
  Controller::propogate_queue();
  check_trigger_count(1,0,1,0);
  D_ASSERT(a.getMin() == 4);
  
  a.setMin(3);
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  D_ASSERT(a.getMin() == 4);
  
  a.setMin(9);
  Controller::propogate_queue();
  check_trigger_count(1,0,1,0);
  D_ASSERT(a.getMin() == 10);

  a.setMin(9);
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  D_ASSERT(a.getMin() == 10);
  
  D_ASSERT(!Controller::failed);
  
  D_ASSERT(a.getMax() == 19);
  a.setMax(22);
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  D_ASSERT(a.getMax() == 19);
  
  a.setMax(19);
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  D_ASSERT(a.getMax() == 19);
  
  a.setMax(18);
  Controller::propogate_queue();
  check_trigger_count(0,1,1,0);
  D_ASSERT(a.getMax() == 18);
  
  a.setMax(16);
  Controller::propogate_queue();
  check_trigger_count(0,1,1,1);
  D_ASSERT(a.getMax() == 10);

  Controller::world_pop();
  D_ASSERT(a.getMin() == 1);
  D_ASSERT(a.getMax() == 19);

  a.propogateAssign(10);
  Controller::propogate_queue();
  check_trigger_count(1,1,1,1);
  D_ASSERT(a.getMax() == 10);
  D_ASSERT(a.getMin() == 10);
  Controller::finish();
}

