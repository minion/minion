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
  
  BigRangeVarRef a;
  a = big_rangevar_container.get_new_var(0,30);
  
  MultiplyVar<BigRangeVarRef> mult_a(a,-3);
  Controller::add_constraint(TestCon(mult_a));
  Controller::lock();
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  D_ASSERT(mult_a.getMin() == -90);
  D_ASSERT(mult_a.getMax() == 0);
  
  mult_a.setMin(-88);
  D_ASSERT(mult_a.getMin() == -87);
  Controller::propogate_queue();
  check_trigger_count(1,0,1,0);
  
  mult_a.setMin(-87);
  D_ASSERT(mult_a.getMin() == -87);
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  
  mult_a.setMin(-86);
  D_ASSERT(mult_a.getMin() == -84);
  Controller::propogate_queue();
  check_trigger_count(1,0,1,0);

  mult_a.setMax(0);
  D_ASSERT(mult_a.getMax() == 0);
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  
  
  mult_a.setMax(-1);
  D_ASSERT(mult_a.getMax() == -3);
  Controller::propogate_queue();
  check_trigger_count(0,1,1,0);

  mult_a.setMax(-2);
  D_ASSERT(mult_a.getMax() == -3);
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);

  Controller::world_push();
  
  mult_a.propogateAssign(-12);
  D_ASSERT(!Controller::failed);
  D_ASSERT(mult_a.getAssignedValue() == -12);
  Controller::propogate_queue();
  check_trigger_count(1,1,1,1);

  Controller::finish();
}

