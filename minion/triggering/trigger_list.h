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

#ifndef TRIGGERLIST_H
#define TRIGGERLIST_H

#include "system/system.h"
#include "solver.h"
#include "triggering/triggers.h"
#include "triggering/constraint_abstract.h"

#include "memory_management/backtrackable_memory.h"
#include "memory_management/nonbacktrack_memory.h"

class TriggerList {

  TriggerList(const TriggerList &);
  TriggerList();
  void operator=(const TriggerList &);
  bool only_bounds;

public:
  TriggerList(bool _only_bounds) : only_bounds(_only_bounds) {
    var_count_m = 0;

    vars_min_domain_val = 0;
    vars_max_domain_val = 0;
    vars_domain_size = 0;
  }

  vector<vector<vector<Trigger>>> triggers;

  vector<vector<DynamicTriggerList>> dynamic_triggers_vec;
  vector<vector<DynamicTriggerList>> dynamic_triggers_domain_vec;

  // void* dynamic_triggers;

  SysInt var_count_m;

  DomainInt vars_min_domain_val;
  DomainInt vars_max_domain_val;
  UnsignedSysInt vars_domain_size;

  void lock(SysInt size, DomainInt min_domain_val, DomainInt max_domain_val) {
    var_count_m = size;
    vars_min_domain_val = min_domain_val;
    vars_max_domain_val = max_domain_val;
    vars_domain_size = checked_cast<UnsignedSysInt>(max_domain_val - min_domain_val + 1);

    dynamic_triggers_vec.resize(4);

    for (int i = 0; i < 4; ++i)
      dynamic_triggers_vec[i].resize(var_count_m);

    if (!only_bounds) {
      dynamic_triggers_domain_vec.resize(vars_domain_size);
      for (int i = 0; i < vars_domain_size; ++i)
        dynamic_triggers_domain_vec[i].resize(var_count_m);
    }

    triggers.resize(4);
    for (UnsignedSysInt i = 0; i < 4; ++i)
      triggers[i].resize(var_count_m);
  }

  pair<Trigger *, Trigger *> get_trigger_range(DomainInt var_num, TrigType type) {
    vector<Trigger> &trigs = triggers[type][checked_cast<SysInt>(var_num)];
    return pair<Trigger *, Trigger *>(trigs.data(), trigs.data() + trigs.size());
  }

  void dynamic_propagate(DomainInt var_num, TrigType type, DomainInt domain_delta,
                         DomainInt val_removed = NoDomainValue) {
    D_ASSERT(val_removed == NoDomainValue ||
             (type == DomainRemoval && val_removed != NoDomainValue));
    D_ASSERT(!only_bounds || type != DomainRemoval);
    DynamicTriggerList *trig;
    if (type != DomainRemoval) {
      trig = &(dynamic_triggers_vec[type][checked_cast<SysInt>(var_num)]);
    } else {
      D_ASSERT(!only_bounds);
      D_ASSERT(vars_min_domain_val <= val_removed);
      D_ASSERT(vars_max_domain_val >= val_removed);
      trig = &(dynamic_triggers_domain_vec[checked_cast<SysInt>(
          val_removed - vars_min_domain_val)][checked_cast<SysInt>(var_num)]);
    }

    // This is an optimisation, no need to push empty lists.
    if (!trig->empty())
      getQueue().pushDynamicTriggers(DynamicTriggerEvent(trig, checked_cast<SysInt>(domain_delta)));
  }

  void push_upper(DomainInt var_num, DomainInt upper_delta) {
    dynamic_propagate(var_num, UpperBound, upper_delta);
    D_ASSERT(upper_delta > 0 || getState().isFailed());

    pair<Trigger *, Trigger *> range = get_trigger_range(var_num, UpperBound);
    if (range.first != range.second)
      getQueue().pushTriggers(
          TriggerRange(range.first, range.second, checked_cast<SysInt>(upper_delta)));
  }

  void push_lower(DomainInt var_num, DomainInt lower_delta) {
    dynamic_propagate(var_num, LowerBound, lower_delta);
    D_ASSERT(lower_delta > 0 || getState().isFailed());
    pair<Trigger *, Trigger *> range = get_trigger_range(var_num, LowerBound);
    if (range.first != range.second)
      getQueue().pushTriggers(
          TriggerRange(range.first, range.second, checked_cast<SysInt>(lower_delta)));
  }

  void push_assign(DomainInt var_num, DomainInt) {
    dynamic_propagate(var_num, Assigned, -1);
    pair<Trigger *, Trigger *> range = get_trigger_range(var_num, Assigned);
    if (range.first != range.second)
      getQueue().pushTriggers(TriggerRange(range.first, range.second, -1));
  }

  void push_domain_changed(DomainInt var_num) {
    dynamic_propagate(var_num, DomainChanged, -1);

    pair<Trigger *, Trigger *> range = get_trigger_range(var_num, DomainChanged);
    if (range.first != range.second)
      getQueue().pushTriggers(TriggerRange(range.first, range.second, -1));
  }

  void push_domain_removal(DomainInt var_num, DomainInt val_removed) {
    D_ASSERT(!only_bounds);
    dynamic_propagate(var_num, DomainRemoval, -1, val_removed);
  }

  void add_domain_trigger(DomainInt b, Trigger t) {
    D_ASSERT(!only_bounds);
    triggers[DomainChanged][checked_cast<SysInt>(b)].push_back(t);
  }

  void add_trigger(DomainInt b, Trigger t, TrigType type) {
    D_ASSERT(type != DomainRemoval);
    triggers[type][checked_cast<SysInt>(b)].push_back(t);
  }

  void addDynamicTrigger(DomainInt b, Trig_ConRef t, TrigType type, DomainInt val,
                         TrigOp op = TO_Default) {
    D_ASSERT(!only_bounds || type != DomainRemoval);
    D_ASSERT(t.con != NULL);

    DynamicTriggerList *queue;

    if (type != DomainRemoval) {
      queue = &dynamic_triggers_vec[type][checked_cast<SysInt>(b)];
    } else {
      D_ASSERT(!only_bounds);
      D_ASSERT(vars_min_domain_val <= val);
      D_ASSERT(vars_max_domain_val >= val);
      queue = &dynamic_triggers_domain_vec[checked_cast<SysInt>(
          val - vars_min_domain_val)][checked_cast<SysInt>(b)];
    }

    D_ASSERT(queue->sanity_check_list());
    /* XXX
        switch (op) {
        case TO_Default: break;
        case TO_Store: break;
        case TO_Backtrack:
          getQueue().getTbq().restoreTriggerOnBacktrack(t);
          // Add to queue.
          t->setQueue(queue);
          break;
        default: abort();
        }
    */

    if (op == TO_Backtrack) {
      getQueue().getTbq().restoreTriggerOnBacktrack(t);
    }

    queue->add(t);
  }
};

inline void attachTriggerToNullList(Trig_ConRef t, TrigOp op) {
  static DynamicTriggerList dt;
  DynamicTriggerList *queue = &dt;

  if (op == TO_Backtrack) {
    getQueue().getTbq().restoreTriggerOnBacktrack(t);
  }

  /* XXX

  switch (op) {
  case TO_Default: D_DATA(t->setQueue((DynamicTriggerList *)BAD_POINTER)); break;
  case TO_Store: t->setQueue(queue); break;
  case TO_Backtrack:
    D_ASSERT(t->getQueue() != (DynamicTriggerList *)BAD_POINTER);
    getQueue().getTbq().addTrigger(t);
    // Add to queue.
    t->setQueue(queue);
    break;
  default: abort();
}*/
  queue->add(t);
}

#endif // TRIGGERLIST_H
