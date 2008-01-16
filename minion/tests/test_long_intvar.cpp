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

void check_domain(BigRangeVarRef& b, int lower, int upper)
{
  D_ASSERT(!b.inDomain(lower - 1));
  D_ASSERT(!b.inDomain(upper + 1));
  for(int i = lower; i <= upper; ++i)
    D_ASSERT(b.inDomain(i));
  
}

int main(void)
{
  BigRangeVarRef b1 = big_rangevar_container.get_new_var(0,1000);
  BigRangeVarRef b2 = big_rangevar_container.get_new_var(100,164);
  BigRangeVarRef b3 = big_rangevar_container.get_new_var(0,65);
  BigRangeVarRef b4 = big_rangevar_container.get_new_var(0,3);
  Controller::lock();
  check_domain(b1,0,1000);
  check_domain(b2,100,164);
  check_domain(b3,0,65);
  check_domain(b4,0,3);
  
  D_ASSERT(b2.getMin() == 100);
  D_ASSERT(b2.getMax() == 164);

  b1.removeFromDomain(500);
 
  D_ASSERT(b1.getMin() == 0);
  D_ASSERT(b1.getMax() == 1000);
  
  check_domain(b1, 0, 499);
  check_domain(b1, 501, 1000);
  check_domain(b2, 100, 164);
  
  b1.setMax(500);
  check_domain(b1,0,499);
  D_ASSERT(b1.getMin() == 0);
  D_ASSERT(b1.getMax() == 499);
  
  check_domain(b2,100,164);
  check_domain(b3,0,65);
  check_domain(b4,0,3);
  
  b2.propogateAssign(164);
  D_ASSERT(b2.getMax() == 164);
  D_ASSERT(b2.getMin() == 164);
  D_ASSERT(b2.isAssigned());
  D_ASSERT(b2.getAssignedValue() == 164);
  
  D_ASSERT(b2.inDomain(164));
  D_ASSERT(!b2.inDomain(163));
  
  check_domain(b3,0,65);
  check_domain(b4,0,3);
  
  b3.propogateAssign(65);
  
  D_ASSERT(b3.inDomain(65));
  D_ASSERT(!b3.inDomain(64));
  
  check_domain(b4,0,3);
  
  Controller::finish();
}

