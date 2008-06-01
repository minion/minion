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

#ifndef DYNAMIC_WATCHED_OR_NEW_H
#define DYNAMIC_WATCHED_OR_NEW_H

#include "../constraints/constraint_abstract.h"
#include "../reversible_vals.h"
#include "../get_info/get_info.h"
#include "../queue/standard_queue.h"

//#define P(x) cout << x << endl;
#define P(x)

  struct Dynamic_OR : public AbstractConstraint
{
  virtual string constraint_name()
    { return "Dynamic OR:"; }

  vector<AbstractConstraint*> cons;

  Reversible<bool> full_propagate_called;
  bool constraint_locked;
  
  int assign_size;

  int propagated_constraint;

  int watched_constraint[2];

  Dynamic_OR(StateObj* _stateObj, vector<AbstractConstraint*> _con) : 
  AbstractConstraint(_stateObj), cons(_con), full_propagate_called(_stateObj, false), assign_size(-1)
    { }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
  /*  DomainInt back_val = *(v + v_size - 1);
    if(back_val != 0)
      return poscon->check_assignment(v, v_size - 1);
    else*/
      return true;
  }

  virtual vector<AnyVarRef> get_vars()
  { 
  /*
    vector<AnyVarRef> vec = poscon->get_vars();
    vec.push_back(rar_var);
    return vec;
    */
    return vector<AnyVarRef>();
  }

  virtual int dynamic_trigger_count() 
  { 
    size_t max_size = 0;
    for(int i = 0; i < cons.size(); ++i)
      max_size = max(max_size, cons[i]->get_vars_singleton()->size());
    assign_size = max_size;
    return max_size * 2;
  }

  // Override setup!
  virtual void setup()
  {
    AbstractConstraint::setup();

    for(int i = 0; i < cons.size(); ++i)
    {
      cons[i]->setup();
      DynamicTrigger* start = cons[i]->dynamic_trigger_start();
      int trigs = cons[i]->dynamic_trigger_count();

      for(int i = 0; i < trigs; ++i)
        (start + i)->constraint = this;
    }
  }

  virtual void special_check()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
    P("Full propagating: " << propagated_constraint);
    cons[propagated_constraint]->full_propagate();
    full_propagate_called = true;
  }

  virtual void special_unlock()
  {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
  }

  PROPAGATE_FUNCTION(DynamicTrigger* trig)
  {
    //PROP_INFO_ADDONE(WatchedOr);
    P("Prop");
    P("Current: " << watched_constraint[0] << " . " << watched_constraint[1]);
    if(constraint_locked)
      return;

    DynamicTrigger* dt = dynamic_trigger_start();

    if(trig >= dt && trig < dt + assign_size * 2)
    {
      int tripped_constraint = (trig - dt) / assign_size;
      int other_constraint = 1 - tripped_constraint;
      P("Tripped: " << tripped_constraint << ":" << watched_constraint[tripped_constraint]);
      D_ASSERT(tripped_constraint == 0 || tripped_constraint == 1);

      GET_ASSIGNMENT(assignment_try, cons[watched_constraint[tripped_constraint]]);
      if(!assignment_try.empty())
      { // Found new support without having to move.
        watch_assignment(cons[watched_constraint[tripped_constraint]], 
                         dt + tripped_constraint * assign_size, assignment_try);
        P("Fixed, returning");
        return; 
      }
      
      for(int i = 0; i < cons.size(); ++i)
      {
        if(i != watched_constraint[0] && i != watched_constraint[1])
        {
          GET_ASSIGNMENT(assignment, cons[i]);
          if(!assignment.empty())
          {
            watch_assignment(cons[i], dt + tripped_constraint * assign_size, assignment);
            watched_constraint[tripped_constraint] = i;
            P("New support. Switch " << tripped_constraint << " to " << i);
            return;
          }
        }
      }
      
      P("Start propagating " << watched_constraint[other_constraint]);
      // Need to propagate!
      propagated_constraint = watched_constraint[other_constraint];
      constraint_locked = true;
	    getQueue(stateObj).pushSpecialTrigger(this);
      return;
    }


    if(full_propagate_called && cons[propagated_constraint]->own_trigger(trig))
    { cons[propagated_constraint]->propagate(trig); }
    else
    {
      // This is an optimisation.
      trig->remove();
    }
  }

  void watch_assignment(AbstractConstraint* con, DynamicTrigger* dt, box<pair<int,int> >& assignment)
  {
    vector<AnyVarRef>& vars = *(con->get_vars_singleton());
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

    while(loop < cons.size() && !found_watch)
    {
      GET_ASSIGNMENT(assignment, cons[loop]);
      if(!assignment.empty())
      {
        found_watch = true;
        watched_constraint[0] = loop;
        watch_assignment(cons[loop], dt, assignment);
      }
      loop++;
    }

    if(found_watch == false)
    {
      getState(stateObj).setFailed(true);
      return;
    }

    P("Found watch 0: " << loop);
    
    found_watch = false;

    while(loop < cons.size() && !found_watch)
    {
      GET_ASSIGNMENT(assignment, cons[loop]);
      if(!assignment.empty())
      {
        found_watch = true;
        watched_constraint[1] = loop;
        watch_assignment(cons[loop], dt + assign_size, assignment);
        P("Found watch 1: " << loop);
        return;
      }
    }

    if(found_watch == false)
    { 
      propagated_constraint = watched_constraint[0];
      constraint_locked = true;
	    getQueue(stateObj).pushSpecialTrigger(this);
    }

  }
};




inline AbstractConstraint*
BuildCT_WATCHED_NEW_OR(StateObj* stateObj, BOOL reify, 
                       const BoolVarRef& reifyVar, ConstraintBlob& bl)
{
  vector<AbstractConstraint*> cons;
  for(int i = 0; i < bl.internal_constraints.size(); ++i)
    cons.push_back(build_dynamic_constraint(stateObj, bl.internal_constraints[i]));


  if(reify) {
    cerr << "Cannot reify 'watched or' constraint." << endl;
    exit(0);
  } else {
    return new Dynamic_OR(stateObj, cons);
  }
}


#endif
