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

#ifndef CHECK_GSA_H_HIUO
#define CHECK_GSA_H_HIUO

#include "constraint_abstract.h"
#include "../memory_management/reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

struct Check_FC : public AbstractConstraint
{
  virtual string constraint_name()
    { return "Check_FC:" + child->constraint_name(); }

  AbstractConstraint* child;

  Check_FC(StateObj* _stateObj, AbstractConstraint* _con) :
  AbstractConstraint(_stateObj), child(_con)
  { }

  virtual ~Check_FC()
    { delete child; }

  virtual int dynamic_trigger_count()
   { return 1; }

  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  { return child->get_satisfying_assignment(assignment); }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  { return child->check_assignment(v, v_size); }

  virtual vector<AnyVarRef> get_vars()
  { return child->get_vars(); }

  virtual void propagate(DynamicTrigger*)
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    int size = child->get_vars_singleton()->size();
    vector<AnyVarRef>* vars = child->get_vars_singleton();
    
    for(int i = 0; i < size; ++i)
    {
        if(!(*vars)[i].isAssigned())
        {
            (*vars)[i].addDynamicTrigger(dt, DomainChanged);
            return;
        }
    }
    
    MAKE_STACK_BOX(b, DomainInt, size);
    for(int i = 0; i < size; ++i)
        b.push_back((*vars)[i].getAssignedValue());
    
    if(!check_assignment(&b[0], size))
        getState(stateObj).setFailed(true);
  }

  virtual void full_propagate()
  { propagate(NULL); }
};

AbstractConstraint*
checkFCCon(StateObj* stateObj, AbstractConstraint* c)
{ return new Check_FC(stateObj, c); }

#endif
