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
  a = rangevar_container.get_new_var(0,6);
  Controller::add_constraint(UnaryNeqCon(a, runtime_val(2)));
  Controller::add_constraint(UnaryNeqCon(a, runtime_val(5)));
  Controller::lock();
  Controller::propogate_queue();
  
  {
	VarIterator<LRangeVarRef> it1(a);
	D_ASSERT(it1.val() == 0);
	D_ASSERT(it1.step());
	D_ASSERT(it1.val() == 1);
	D_ASSERT(it1.step());
	D_ASSERT(it1.val() == 3);
	D_ASSERT(it1.step());
	D_ASSERT(it1.val() == 4);
	D_ASSERT(it1.step());
	D_ASSERT(it1.val() == 6);
	D_ASSERT(!it1.step());
  }
  {
	VarIterator<LRangeVarRef> it2(a, 1);
	D_ASSERT(it2.val() == 1);
  }
  {
	VarIterator<LRangeVarRef> it3(a, 2);
	D_ASSERT(it3.val() == 3);
  }
}

