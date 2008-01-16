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


int test_lower = 0;
int test_upper = 0;
int test_domainchanged = 0;
int test_assigned = 0;

void check_trigger_count(int i, int j, int k, int l)
{
  D_ASSERT(i == test_lower);
  D_ASSERT(j == test_upper);
  D_ASSERT(k == test_domainchanged);
  D_ASSERT(l == test_assigned);
  test_lower = test_upper = test_domainchanged = test_assigned = 0;
}


template<typename VarRef>
struct TestConstraint : public Constraint
{
  virtual string constraint_name()
  { return "TestConstraint"; }
  
  VarRef var1;
  TestConstraint(VarRef _var1) :
    var1(_var1)
  {}
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_ANDCON,"Setting up Constraint");
    triggerCollection t;
	t.push_back(make_trigger(var1, Trigger(this, 1), LowerBound));
	t.push_back(make_trigger(var1, Trigger(this, 2), UpperBound));
	t.push_back(make_trigger(var1, Trigger(this, 3), DomainChanged));
	t.push_back(make_trigger(var1, Trigger(this, 4), Assigned));
    return t;
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
    switch(i)
    {
      case 1:
	test_lower++;
	break;
      case 2:
	test_upper++;
	break;
      case 3:
	test_domainchanged++;
	break;
      case 4:
	test_assigned++;
	break;
      default:
	FAIL_EXIT();
    }
  }
  
  virtual void full_propogate()
  {}
  
  
};

template<typename VarRef>
Constraint*
TestCon(VarRef var1)
{ 
  return (new TestConstraint<VarRef>(var1)); 
}

