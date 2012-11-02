/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
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

#ifndef CHECK_ASSIGN_H_HIUO
#define CHECK_ASSIGN_H_HIUO

#include "constraint_abstract.h"
#include "../memory_management/reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

struct Check_Assign : public AbstractConstraint
{

  virtual string extended_name()
  { return constraint_name() + ":" + child->extended_name(); }

  virtual string constraint_name()
  { return "check[assign]"; }

  CONSTRAINT_ARG_LIST1(child);

  AbstractConstraint* child;

  Check_Assign(StateObj* _stateObj, AbstractConstraint* _con) :
  AbstractConstraint(_stateObj), child(_con)
  { }

  virtual AbstractConstraint* reverse_constraint()
  {
    return new Check_Assign(stateObj, child->reverse_constraint());
  }


  virtual ~Check_Assign()
    { delete child; }

  virtual SysInt dynamic_trigger_count()
   { return 1; }

  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  { return child->get_satisfying_assignment(assignment); }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  { return child->check_assignment(v, v_size); }

  virtual vector<AnyVarRef> get_vars()
  { return child->get_vars(); }

  virtual void propagate(DynamicTrigger*)
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    SysInt size = child->get_vars_singleton()->size();
    vector<AnyVarRef>* vars = child->get_vars_singleton();
    
    for(SysInt i = 0; i < size; ++i)
    {
        if(!(*vars)[i].isAssigned())
        {
            (*vars)[i].addDynamicTrigger(dt, DomainChanged);
            return;
        }
    }
    
    MAKE_STACK_BOX(b, DomainInt, size);
    for(SysInt i = 0; i < size; ++i)
        b.push_back((*vars)[i].getAssignedValue());
    
    if(!check_assignment(&b[0], size))
        getState(stateObj).setFailed(true);
  }

  virtual void full_propagate()
  { propagate(NULL); }
};

inline AbstractConstraint*
checkAssignCon(StateObj* stateObj, AbstractConstraint* c)
{ return new Check_Assign(stateObj, c); }

#endif
