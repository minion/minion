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



int main(void)
{
  LRangeVarRef a;
  a = rangevar_container.get_new_var(0,31);
  NegType<LRangeVarRef>::type neg_a(a);
  
  vector<LRangeVarRef> vec_a(1);
  array<LRangeVarRef,1> array_a;
  vec_a[0] = a;
  array_a[0] = a;
  
  NegType<vector<LRangeVarRef> >::type neg_vec_a;
  NegType<array<LRangeVarRef,1> >::type neg_array_a;
  
  neg_vec_a = VarNegRef(vec_a);
  neg_array_a = VarNegRef(array_a);
  
  Controller::add_constraint(TestCon(neg_a));
  Controller::lock();
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  D_ASSERT(neg_a.getMax() == 0);
  D_ASSERT(neg_a.getMin() == -31);
  
  D_ASSERT(neg_vec_a[0].getMin() == -31);
  D_ASSERT(neg_array_a[0].getMin() == -31);
  
  Controller::world_push();
  neg_a.setMax(-5);
  Controller::propogate_queue();
  check_trigger_count(0,1,1,0);
  D_ASSERT(a.getMin() == 5);
  D_ASSERT(a.getMax() == 31);
  Controller::finish();
}

