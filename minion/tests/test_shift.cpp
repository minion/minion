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
 
  BoolVarRef a;
  typedef ShiftType<NegType<BoolVarRef>::type, compiletime_val<-1> >::type VarType;
  a = boolean_container.get_new_var();
  VarType b = ShiftVarRef(VarNegRef(a), compiletime_val<-1>());

  Controller::add_constraint(TestCon(b));
  Controller::lock();
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  D_ASSERT(b.getMin() == -2);
  D_ASSERT(b.getMax() == -1);
  D_ASSERT(b.inDomain(-2));
  D_ASSERT(b.inDomain(-1));
  D_ASSERT(!b.inDomain(0));
  D_ASSERT(!b.inDomain(-3));
  b.setMin(-2);
  D_ASSERT(b.getMin() == -2);
  Controller::propogate_queue();
  check_trigger_count(0,0,0,0);
  
  Controller::world_push();
  
  b.setMin(-1);
  D_ASSERT(!Controller::failed);
  D_ASSERT(b.getMin() == -1);
  D_ASSERT(b.getAssignedValue() == -1);
  
  Controller::propogate_queue();
  check_trigger_count(1,0,1,1);
  
  b.setMin(0);
  D_ASSERT(Controller::failed);
  Controller::failed = false;
  Controller::world_pop();
  
  b.propogateAssign(-1);
  D_ASSERT(!Controller::failed);
  D_ASSERT(b.getMin() == -1);
  D_ASSERT(b.getMax() == -1);
  D_ASSERT(b.getAssignedValue() == -1);
  D_ASSERT(b.inDomain(-1));
  D_ASSERT(!b.inDomain(0));
  D_ASSERT(!b.inDomain(-2));
  
  Controller::finish();
}

