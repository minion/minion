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

#include "solver.h"
#include "system/system.h"
#include "triggering/constraint_abstract.h"
#include "triggering/triggers.h"

#include "memory_management/MemoryBlock.h"
#include "memory_management/nonbacktrack_memory.h"

#define MAX_VARS 1000000

struct TriggerObj {
  DomainInt min;
  DomainInt max;
  vector<DynamicTriggerList> _dynamic_triggers;

  TriggerObj() : min(-1), max(-1) {}

  DynamicTriggerList* trigger_type(DomainInt type) {
    D_ASSERT(type >= 0 && type < 4);
    return &_dynamic_triggers[checked_cast<SysInt>(type)];
  }

  DynamicTriggerList* domain_val(DomainInt val) {
    D_ASSERT(val >= min && val <= max);
    return &_dynamic_triggers[checked_cast<SysInt>(val - min + 4)];
  }
};

class TriggerList {

  TriggerList(const TriggerList&);
  TriggerList();
  void operator=(const TriggerList&);
  bool only_bounds;

public:
  TriggerList(bool _only_bounds) : only_bounds(_only_bounds) {
    var_count_m = 0;
  }

  vector<TriggerObj> dyn_triggers;

  //  vector<vector<DynamicTriggerList>> dynamic_triggers_vec;
  //  vector<vector<DynamicTriggerList>> dynamic_triggers_domain_vec;

  // void* dynamic_triggers;

  SysInt var_count_m;

  //  std::vector<std::pair<DomainInt, DomainInt> > vars_domain;

  void lock(SysInt size, DomainInt min_domain_val, DomainInt max_domain_val) {
    std::vector<std::pair<DomainInt, DomainInt>> doms(size,
                                                      make_pair(min_domain_val, max_domain_val));
    addVariables(doms);
  }

  void addVariables(const std::vector<pair<DomainInt, DomainInt>>& doms) {
    SysInt old_var_count = var_count_m;
    var_count_m += doms.size();
    CHECK(var_count_m < MAX_VARS, "Too many variables... increase MAX_VARS");
    dyn_triggers.resize(var_count_m);
    for(int i = 0; i < doms.size(); ++i) {
      dyn_triggers[old_var_count + i].min = doms[i].first;
      dyn_triggers[old_var_count + i].max = doms[i].second;
      if(only_bounds)
        dyn_triggers[old_var_count + i]._dynamic_triggers.resize(4);
      else
        dyn_triggers[old_var_count + i]._dynamic_triggers.resize(
            checked_cast<SysInt>(4 + (doms[i].second - doms[i].first + 1)));
    }
  }

  void dynamic_propagate(DomainInt _var_num, TrigType type, DomainInt domain_delta,
                         DomainInt val_removed = NoDomainValue) {
    const SysInt var_num = checked_cast<SysInt>(_var_num);
    D_ASSERT(val_removed == NoDomainValue ||
             (type == DomainRemoval && val_removed != NoDomainValue));
    D_ASSERT(!only_bounds || type != DomainRemoval);
    DynamicTriggerList* trig;
    if(type != DomainRemoval) {
      trig = dyn_triggers[var_num].trigger_type(type);
    } else {
      D_ASSERT(!only_bounds);
      D_ASSERT(dyn_triggers[var_num].min <= val_removed);
      D_ASSERT(dyn_triggers[var_num].max >= val_removed);
      trig = dyn_triggers[var_num].domain_val(val_removed);
    }

    // This is an optimisation, no need to push empty lists.
    if(!trig->empty())
      getQueue().pushDynamicTriggers(DynamicTriggerEvent(trig, checked_cast<SysInt>(domain_delta)));
  }

  void push_upper(DomainInt var_num, DomainInt upper_delta) {
    D_ASSERT(upper_delta > 0 || getState().isFailed());
    dynamic_propagate(var_num, UpperBound, upper_delta);
  }

  void push_lower(DomainInt var_num, DomainInt lower_delta) {
    D_ASSERT(lower_delta > 0 || getState().isFailed());
    dynamic_propagate(var_num, LowerBound, lower_delta);
  }

  void push_assign(DomainInt var_num, DomainInt) {
    dynamic_propagate(var_num, Assigned, -1);
  }

  void push_domain_changed(DomainInt var_num) {
    dynamic_propagate(var_num, DomainChanged, -1);
  }

  void push_domain_removal(DomainInt var_num, DomainInt val_removed) {
    D_ASSERT(!only_bounds);
    dynamic_propagate(var_num, DomainRemoval, -1, val_removed);
  }

  void addDynamicTrigger(DomainInt _b, Trig_ConRef t, TrigType type, DomainInt val,
                         TrigOp op = TO_Default) {
    const SysInt b = checked_cast<SysInt>(_b);
    D_ASSERT(!only_bounds || type != DomainRemoval);
    D_ASSERT(t.con != NULL);

    DynamicTriggerList* queue;

    if(type != DomainRemoval) {
      queue = dyn_triggers[b].trigger_type(type);
    } else {
      D_ASSERT(!only_bounds);
      D_ASSERT(dyn_triggers[b].min <= val);
      D_ASSERT(dyn_triggers[b].max >= val);
      queue = dyn_triggers[b].domain_val(val);
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

    if(op == TO_Backtrack) {
      getQueue().getTbq().restoreTriggerOnBacktrack(t);
    }

    queue->add(t);
  }
};

inline void attachTriggerToNullList(Trig_ConRef t, TrigOp op) {
  static DynamicTriggerList dt;
  DynamicTriggerList* queue = &dt;

  if(op == TO_Backtrack) {
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
