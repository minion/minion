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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

/** @help constraints;watched-or Description
The constraint

  watched-or({C1,...,Cn})

ensures that at least one of the constraints C1,...,Cn is true.
*/

/** @help constraints;watched-or References
  See also

  help constraints watched-and
*/

#ifndef DYNAMIC_WATCHED_OR_NEW_H
#define DYNAMIC_WATCHED_OR_NEW_H

#include "../get_info/get_info.h"
#include "../memory_management/reversible_vals.h"
#include "../queue/standard_queue.h"
#include "../triggering/constraint_abstract.h"

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl;
#define P(x)

struct Dynamic_OR : public ParentConstraint {
  virtual string constraint_name() {
    return "watched-or";
  }

  CONSTRAINT_ARG_LIST1(child_constraints);

  Reversible<bool> full_propagate_called;
  bool constraint_locked;

  SysInt assign_size;

  SysInt propagated_constraint;

  SysInt watched_constraint[2];

  Dynamic_OR(vector<AbstractConstraint*> _con)
      : ParentConstraint(_con),
        full_propagate_called(false),
        constraint_locked(false),
        assign_size(-1),
        propagated_constraint(-1) {
    size_t max_size = 0;
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i)
      max_size = max(max_size, child_constraints[i]->get_vars_singleton()->size());
    assign_size = max_size * 2;
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size) {
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i) {
      if(child_constraints[i]->check_assignment(v + checked_cast<SysInt>(start_of_constraint[i]),
                                                child_constraints[i]->get_vars_singleton()->size()))
        return true;
    }
    return false;
  }

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>>& assignment) {
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i) {
      assignment.clear();
      bool flag = child_constraints[i]->get_satisfying_assignment(assignment);
      if(flag) {
        // Fix up assignment
        for(SysInt j = 0; j < (SysInt)assignment.size(); ++j) {
          assignment[j].first += checked_cast<SysInt>(start_of_constraint[i]);
          D_ASSERT((*(child_constraints[i]
                          ->get_vars_singleton()))[checked_cast<SysInt>(assignment[j].first -
                                                                        start_of_constraint[i])]
                       .inDomain(assignment[j].second));
          D_ASSERT(
              (*(this->get_vars_singleton()))[checked_cast<SysInt>(assignment[j].first)].inDomain(
                  assignment[j].second));
        }
        return true;
      }
    }
    return false;
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> vecs;
    for(SysInt i = 0; i < (SysInt)child_constraints.size(); ++i) {
      vector<AnyVarRef>* var_ptr = child_constraints[i]->get_vars_singleton();
      vecs.insert(vecs.end(), var_ptr->begin(), var_ptr->end());
    }
    return vecs;
  }

  virtual SysInt dynamic_trigger_count() {
    return assign_size * 2;
  }

  virtual void special_check() {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
    P("Full propagating: " << propagated_constraint);
    child_constraints[propagated_constraint]->full_propagate();
    full_propagate_called = true;
  }

  virtual void special_unlock() {
    D_ASSERT(constraint_locked);
    constraint_locked = false;
  }

  virtual void propagateDynInt(SysInt trig, DomainDelta dd) {
    // PROP_INFO_ADDONE(WatchedOr);
    P("Prop");
    P("Current: " << watched_constraint[0] << " . " << watched_constraint[1]);
    P("FullPropOn: " << (bool)full_propagate_called << ", on: " << propagated_constraint);
    P("Locked:" << constraint_locked);
    if(constraint_locked)
      return;

    if(trig >= 0 && trig < assign_size * 2) {
      if(full_propagate_called)
        return;

      SysInt tripped_constraint = checked_cast<SysInt>(trig / assign_size);
      SysInt other_constraint = checked_cast<SysInt>(1 - tripped_constraint);
      P("Tripped: " << tripped_constraint << ":" << watched_constraint[tripped_constraint]);
      D_ASSERT(tripped_constraint == 0 || tripped_constraint == 1);

      bool flag;
      GET_ASSIGNMENT(assignment_try, child_constraints[watched_constraint[tripped_constraint]]);
      if(flag) { // Found new support without having to move.
        watch_assignment(child_constraints[watched_constraint[tripped_constraint]],
                         tripped_constraint * assign_size, assignment_try);
        for(SysInt i = 0; i < (SysInt)assignment_try.size(); ++i)
          P(assignment_try[i].first << "." << assignment_try[i].second << "  ");
        P(" -- Fixed, returning");
        return;
      }

      const SysInt cons_s = child_constraints.size();

      SysInt loop_start = watched_constraint[tripped_constraint] + 1;
      SysInt skip_pos = watched_constraint[other_constraint];

      for(SysInt i = loop_start; i < cons_s; ++i) {
        if(i != skip_pos) {
          GET_ASSIGNMENT(assignment, child_constraints[i]);
          if(flag) {
            watch_assignment(child_constraints[i], tripped_constraint * assign_size, assignment);
            watched_constraint[tripped_constraint] = i;
            P("New support. Switch " << tripped_constraint << " to " << i);
            return;
          }
        }
      }

      for(SysInt i = 0; i < loop_start - 1; ++i) {
        if(i != skip_pos) {
          GET_ASSIGNMENT(assignment, child_constraints[i]);
          if(flag) {
            watch_assignment(child_constraints[i], tripped_constraint * assign_size, assignment);
            watched_constraint[tripped_constraint] = i;
            P("New support. Switch " << tripped_constraint << " to " << i);
            return;
          }
        }
      }

      P("Start propagating " << watched_constraint[other_constraint]);
      // Need to propagate!
      propagated_constraint = watched_constraint[other_constraint];
      // the following may be necessary for correctness for some constraints
      constraint_locked = true;
      getQueue().pushSpecialTrigger(this);
      return;
    }

    if(full_propagate_called && getChildDynamicTrigger(trig) == propagated_constraint) {
      P("Propagating child");
      passDynTriggerToChild(trig, dd);
      // child_constraints[propagated_constraint]->propagateDynInt(trig);
    } else {
      P("Clean old trigger");
      // This is an optimisation.
      releaseTriggerInt(trig);
    }
  }

  void watch_assignment(AbstractConstraint* con, DomainInt dt,
                        box<pair<SysInt, DomainInt>>& assignment) {
    vector<AnyVarRef>& vars = *(con->get_vars_singleton());
    D_ASSERT((SysInt)assignment.size() <= assign_size);
    for(SysInt i = 0; i < (SysInt)assignment.size(); ++i) {
      const SysInt af = checked_cast<SysInt>(assignment[i].first);
      if(vars[af].isBound())
        moveTriggerInt(vars[af], dt + i, DomainChanged);
      else
        moveTriggerInt(vars[af], dt + i, DomainRemoval, assignment[i].second);
    }
  }

  virtual void full_propagate() {
    P("Full Propagate")
    // Clean up triggers
    for(SysInt i = 0; i < assign_size * 2; ++i)
      releaseTriggerInt(i);

    SysInt loop = 0;

    bool found_watch = false;

    while(loop < (SysInt)child_constraints.size() && !found_watch) {
      bool flag;
      GET_ASSIGNMENT(assignment, child_constraints[loop]);
      if(flag) {
        found_watch = true;
        watched_constraint[0] = loop;
        watch_assignment(child_constraints[loop], 0, assignment);
        for(SysInt i = 0; i < (SysInt)assignment.size(); ++i)
          P(assignment[i].first << "." << assignment[i].second << "  ");
      } else
        loop++;
    }

    if(found_watch == false) {
      getState().setFailed(true);
      return;
    }

    P(" -- Found watch 0: " << loop);
    loop++;

    found_watch = false;

    while(loop < (SysInt)child_constraints.size() && !found_watch) {
      bool flag;
      GET_ASSIGNMENT(assignment, child_constraints[loop]);
      if(flag) {
        found_watch = true;
        watched_constraint[1] = loop;
        watch_assignment(child_constraints[loop], assign_size, assignment);
        for(SysInt i = 0; i < (SysInt)assignment.size(); ++i)
          P(assignment[i].first << "." << assignment[i].second << "  ");
        P(" -- Found watch 1: " << loop);
        return;
      } else
        loop++;
    }

    if(found_watch == false) {
      propagated_constraint = watched_constraint[0];
      constraint_locked = true;
      getQueue().pushSpecialTrigger(this);
    }
  }

  virtual AbstractConstraint* reverse_constraint();
};

#include "dynamic_new_and.h"

inline AbstractConstraint* Dynamic_OR::reverse_constraint() { // and of the reverse of all the child
                                                              // constraints..
  vector<AbstractConstraint*> con;
  for(SysInt i = 0; i < (SysInt)child_constraints.size(); i++) {
    con.push_back(child_constraints[i]->reverse_constraint());
  }
  return new Dynamic_AND(con);
}

inline AbstractConstraint* BuildCT_WATCHED_NEW_OR(ConstraintBlob& bl) {
  vector<AbstractConstraint*> cons;
  for(SysInt i = 0; i < (SysInt)bl.internal_constraints.size(); ++i)
    cons.push_back(build_constraint(bl.internal_constraints[i]));
  return new Dynamic_OR(cons);
}

/* JSON
{ "type": "constraint",
  "name": "watched-or",
  "internal_name": "CT_WATCHED_NEW_OR",
  "args": [ "read_constraint_list" ]
}
*/

#endif
