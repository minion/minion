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

/** @help constraints;watched-and Description
The constraint

  watched-and({C1,...,Cn})

ensures that the constraints C1,...,Cn are all true.
*/

/** @help constraints;watched-and Notes Conjunctions of constraints may seem
pointless, bearing in mind that a CSP is simply a conjunction of constraints
already! However sometimes it may be necessary to use a conjunction as a child
of another constraint, for example in a reification:

   reify(watched-and({...}),r)
*/

/** @help constraints;watched-and References
  See also

  help constraints watched-or
*/

#ifndef DYNAMIC_WATCHED_AND_NEW_H
#define DYNAMIC_WATCHED_AND_NEW_H

#include "../triggering/constraint_abstract.h"
#include "../memory_management/reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"



#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

// Similar to watched or, but has no watching phase, just propagates all
// the time, and propagates all constraints of course.


struct Dynamic_AND : public ParentConstraint
{
  virtual string constraint_name()
  { return "watched-and"; }

  CONSTRAINT_ARG_LIST1(child_constraints);

  bool constraint_locked;
  SysInt propagated_to;
  Dynamic_AND(vector<AbstractConstraint*> _con) :
    ParentConstraint(_con), constraint_locked(false)
    { }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i)
    {
      if(! child_constraints[i]->check_assignment(v + checked_cast<SysInt>(start_of_constraint[i]),
         child_constraints[i]->get_vars_singleton()->size()))
         return false;
    }
    return true;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {
      // get all satisfying assignments of child constraints and stick
      // them together. Even if they contradict each other.
      typedef pair<SysInt, DomainInt> temptype;
      MAKE_STACK_BOX(localassignment, temptype, assignment.capacity());
      P("GetSat for And");
      for(SysInt i=0; i<(SysInt)child_constraints.size(); ++i)
      {
          localassignment.clear();
          bool flag=child_constraints[i]->get_satisfying_assignment(localassignment);
          if(!flag)
          {
              assignment.clear();
              return false;
          }
          P(localassignment[0] << ":" << localassignment[1]);
          for(SysInt j=0; j < (SysInt)localassignment.size(); j++)
          {
              assignment.push_back(make_pair(checked_cast<SysInt>(localassignment[j].first+start_of_constraint[i]),
                  localassignment[j].second));
              D_ASSERT((*(this->get_vars_singleton()))[checked_cast<SysInt>(localassignment[j].first+start_of_constraint[i])].inDomain(localassignment[j].second));
              D_ASSERT((*(child_constraints[i]->get_vars_singleton()))[checked_cast<SysInt>(localassignment[j].first)].inDomain(localassignment[j].second));
          }
      }
      return true;
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vecs;
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i)
    {
      vector<AnyVarRef>* var_ptr = child_constraints[i]->get_vars_singleton();
      vecs.insert(vecs.end(), var_ptr->begin(), var_ptr->end());
    }
    return vecs;
  }

  virtual SysInt dynamic_trigger_count()
  {
    return 0;
  }

  virtual void special_check()
  {
    D_ASSERT(constraint_locked);
    P("Full propagating all constraints in AND");
    if(child_constraints.size() == 0)
    {
      constraint_locked = false;
      return;
    }

    child_constraints[propagated_to]->full_propagate();
    propagated_to++;
    if(propagated_to != (SysInt)child_constraints.size())
      getQueue().pushSpecialTrigger(this);
    else
    {
       constraint_locked = false;
    }
  }

  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
  }

  virtual void propagateStatic(DomainInt i, DomainDelta domain)
  {
    //PROP_INFO_ADDONE(WatchedOR);
    P("Static propagate start");
    pair<DomainInt, DomainInt> childTrigger = getChildStaticTrigger(i);
    P("Got trigger: " << i << ", maps to: " << childTrigger.first << "." << childTrigger.second);
    P("Passing trigger " << childTrigger.second << " on");
    if(!constraint_locked || childTrigger.first < propagated_to)
    child_constraints[checked_cast<SysInt>(childTrigger.first)]->propagateStatic(childTrigger.second, domain);
  }

  virtual void propagateDynInt(SysInt  trig)
  {
    //PROP_INFO_ADDONE(WatchedOr);
    P("Prop");
    P("Locked:" << constraint_locked);
    // pass the trigger down
    P("Propagating child");
    // need to know which child to prop.
    SysInt child = getChildDynamicTrigger(trig);
    if(!constraint_locked || child < propagated_to)
    {
      passDynTriggerToChild(trig);
      //child_constraints[child]->propagateDynInt(trig);
    }
  }

  virtual void full_propagate()
  {
    P("AND Full Propagate");
    // push it on the special queue to be full_propagated later.
    D_ASSERT(!constraint_locked);
    constraint_locked = true;
    propagated_to = 0;
    getQueue().pushSpecialTrigger(this);
  }

  virtual AbstractConstraint* reverse_constraint();
};

#include "dynamic_new_or.h"

inline AbstractConstraint* Dynamic_AND::reverse_constraint()
{ // OR of the reverse of all the child constraints..
  vector<AbstractConstraint*> con;
  for(SysInt i=0; i < (SysInt)child_constraints.size(); i++)
  {
      con.push_back(child_constraints[i]->reverse_constraint());
  }
  return new Dynamic_OR(con);
}

inline AbstractConstraint*
BuildCT_WATCHED_NEW_AND(ConstraintBlob& bl)
{
  vector<AbstractConstraint*> cons;
  for(SysInt i = 0; i < (SysInt)bl.internal_constraints.size(); ++i)
    cons.push_back(build_constraint(bl.internal_constraints[i]));

  return new Dynamic_AND(cons);
}

/* JSON
{ "type": "constraint",
  "name": "watched-and",
  "internal_name": "CT_WATCHED_NEW_AND",
  "args": [ "read_constraint_list" ]
}
*/
#endif
