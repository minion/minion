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

#define SLOW_WOR

#ifndef DYNAMIC_WATCHED_OR_NEW_H
#define DYNAMIC_WATCHED_OR_NEW_H

#include "../constraints/constraint_abstract.h"
#include "../reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"

#ifdef P
#undef P
#endif

#define P(x) cout << x << endl;
//#define P(x)

// For reverse_constraint we need an and
#include "dynamic_new_and.h"

struct Dynamic_OR : public ParentConstraint
{
  virtual string constraint_name()
    { return "Dynamic OR:"; }


  Reversible<bool> full_propagate_called;
  bool constraint_locked;
  
  int assign_size;

  int propagated_constraint;

  int watched_constraint[2];

  Dynamic_OR(StateObj* _stateObj, vector<AbstractConstraint*> _con) : 
    ParentConstraint(_stateObj, _con), full_propagate_called(_stateObj, false), constraint_locked(false),
    assign_size(-1), propagated_constraint(-1)
    {
      size_t max_size = 0;
      for(int i = 0; i < child_constraints.size(); ++i)
        max_size = max(max_size, child_constraints[i]->get_vars_singleton()->size());
      assign_size = max_size * 2;
    }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    for(int i = 0; i < child_constraints.size(); ++i)
    {
      if(child_constraints[i]->check_assignment(v + start_of_constraint[i],
         child_constraints[i]->get_vars_singleton()->size()))
         return true;
    }
    return false;
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    for(int i = 0; i < child_constraints.size(); ++i)
    {
      bool flag=child_constraints[i]->get_satisfying_assignment(assignment);
      if(flag)
      {
        // Fix up assignment
        for(int j = 0; j < assignment.size(); ++j)
          assignment[j].first += start_of_constraint[i];
        return true; 
      }
    }
    return false;
  }
  

  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vecs;
    for(int i = 0; i < child_constraints.size(); ++i)
    {
      vector<AnyVarRef>* var_ptr = child_constraints[i]->get_vars_singleton(); 
      vecs.insert(vecs.end(), var_ptr->begin(), var_ptr->end());
    }
    return vecs;
  }

  virtual int dynamic_trigger_count() 
  { 
    return assign_size * 2;
  }

  virtual void special_check()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
    P("Full propagating: " << propagated_constraint);
    child_constraints[propagated_constraint]->full_propagate();
    full_propagate_called = true;
  }

  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
  }

  PROPAGATE_FUNCTION(int i, DomainDelta domain)
  {
    //PROP_INFO_ADDONE(WatchedOR);
    P("Static propagate start");
    if(constraint_locked)
      return;

    if(full_propagate_called)
    {
      P("Already doing static full propagate");
      pair<int,int> childTrigger = getChildStaticTrigger(i);
      P("Got trigger: " << i << ", maps to: " << childTrigger.first << "." << childTrigger.second);
      if(childTrigger.first == propagated_constraint)
      {
        P("Passing trigger" << childTrigger.second << "on");
        child_constraints[propagated_constraint]->propagate(childTrigger.second, domain);
      }
    }
  }
  
  PROPAGATE_FUNCTION(DynamicTrigger* trig)
  {
    //PROP_INFO_ADDONE(WatchedOr);
    P("Prop");
    P("Current: " << watched_constraint[0] << " . " << watched_constraint[1]);
    P("FullPropOn: " << (bool)full_propagate_called << ", on: " << propagated_constraint);
    P("Locked:" << constraint_locked);
    if(constraint_locked)
      return;

    DynamicTrigger* dt = dynamic_trigger_start();

    P("Trig: " << trig - dt);

    if(trig >= dt && trig < dt + assign_size * 2)
    {
      if(full_propagate_called)
        return;

      int tripped_constraint = (trig - dt) / assign_size;
      int other_constraint = 1 - tripped_constraint;
      P("Tripped: " << tripped_constraint << ":" << watched_constraint[tripped_constraint]);
      D_ASSERT(tripped_constraint == 0 || tripped_constraint == 1);
      
      bool flag;
      GET_ASSIGNMENT(assignment_try, child_constraints[watched_constraint[tripped_constraint]]);
      if(flag)
      { // Found new support without having to move.
        watch_assignment(child_constraints[watched_constraint[tripped_constraint]], 
                         dt + tripped_constraint * assign_size, assignment_try);
        for(int i = 0; i < assignment_try.size(); ++i)
          P(assignment_try[i].first << "." << assignment_try[i].second << "  ");
        P(" -- Fixed, returning");
        return; 
      }
      
      const size_t cons_s = child_constraints.size();
      for(int i = 0; i < cons_s; ++i)
      {
        if(i != watched_constraint[0] && i != watched_constraint[1])
        {
          GET_ASSIGNMENT(assignment, child_constraints[i]);
          if(flag)
          {
            watch_assignment(child_constraints[i], dt + tripped_constraint * assign_size, assignment);
            watched_constraint[tripped_constraint] = i;
            P("New support. Switch " << tripped_constraint << " to " << i);
            return;
          }
        }
      }
      
      P("Start propagating " << watched_constraint[other_constraint]);
      // Need to propagate!
      propagated_constraint = watched_constraint[other_constraint];
      //the following may be necessary for correctness for some constraints
#ifdef SLOW_WOR
      constraint_locked = true;
      getQueue(stateObj).pushSpecialTrigger(this);
#else
      child_constraints[propagated_constraint]->full_propagate();
      full_propagate_called = true;
#endif
      return;
    }


    if(full_propagate_called && getChildDynamicTrigger(trig) == propagated_constraint)
    { 
      P("Propagating child");
      child_constraints[propagated_constraint]->propagate(trig); 
    }
    else
    {
      P("Clean old trigger");
      // This is an optimisation.
      trig->remove();
    }
  }

  void watch_assignment(AbstractConstraint* con, DynamicTrigger* dt, box<pair<int,DomainInt> >& assignment)
  {
    vector<AnyVarRef>& vars = *(con->get_vars_singleton());
    D_ASSERT(assignment.size() <= assign_size);
    for(int i = 0; i < assignment.size(); ++i)
      vars[assignment[i].first].addDynamicTrigger(dt + i, DomainRemoval, assignment[i].second);
  }

  virtual void full_propagate()
  {
    P("Full Propagate")
    DynamicTrigger* dt = dynamic_trigger_start();

    // Clean up triggers
    for(int i = 0; i < assign_size * 2; ++i)
      dt[i].remove();

    int loop = 0;

    bool found_watch = false;
    
    while(loop < child_constraints.size() && !found_watch)
    {
      bool flag;
      GET_ASSIGNMENT(assignment, child_constraints[loop]);
      if(flag)
      {
        found_watch = true;
        watched_constraint[0] = loop;
        watch_assignment(child_constraints[loop], dt, assignment);
        for(int i = 0; i < assignment.size(); ++i)
          P(assignment[i].first << "." << assignment[i].second << "  ");
      }
      else
        loop++;
    }

    if(found_watch == false)
    {
      getState(stateObj).setFailed(true);
      return;
    }

    P(" -- Found watch 0: " << loop);
    loop++;
    
    found_watch = false;
    
    while(loop < child_constraints.size() && !found_watch)
    {
      bool flag;
      GET_ASSIGNMENT(assignment, child_constraints[loop]);
      if(flag)
      {
        found_watch = true;
        watched_constraint[1] = loop;
        watch_assignment(child_constraints[loop], dt + assign_size, assignment);
        for(int i = 0; i < assignment.size(); ++i)
          P(assignment[i].first << "." << assignment[i].second << "  ");
        P(" -- Found watch 1: " << loop);
        return;
      }
      else
        loop++;
    }

    if(found_watch == false)
    { 
      propagated_constraint = watched_constraint[0];
      constraint_locked = true;
	    getQueue(stateObj).pushSpecialTrigger(this);
    }

  }
  
  virtual AbstractConstraint* reverse_constraint()
  { // and of the reverse of all the child constraints..
      vector<AbstractConstraint*> con;
      for(int i=0; i<child_constraints.size(); i++)
      {
          con.push_back(child_constraints[i]->reverse_constraint());
      }
      return new Dynamic_AND(stateObj, con);
  }
};

inline AbstractConstraint*
BuildCT_WATCHED_NEW_OR(StateObj* stateObj, BOOL reify, 
                       const BoolVarRef& reifyVar, ConstraintBlob& bl)
{
  vector<AbstractConstraint*> cons;
  for(int i = 0; i < bl.internal_constraints.size(); ++i)
    cons.push_back(build_constraint(stateObj, bl.internal_constraints[i]));


  if(reify) {
    cerr << "Cannot reify 'watched or' constraint." << endl;
    exit(0);
  } else {
    return new Dynamic_OR(stateObj, cons);
  }
}


#endif
