// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

#ifndef TRIGGERLIST_H
#define TRIGGERLIST_H

#include "solver.h"
#include "system/system.h"
#include "triggering/constraint_abstract.h"
#include "triggering/triggers.h"

#include "memory_management/MemoryBlock.h"
#include "memory_management/nonbacktrack_memory.h"

struct TriggerObj {
  DomainInt min;
  DomainInt max;
  vector<DynamicTriggerList> _dynamicTriggers;

  TriggerObj() : min(-1), max(-1) {}

  DynamicTriggerList* trigger_type(DomainInt type) {
    D_ASSERT(type >= 0 && type < 4);
    return &_dynamicTriggers[checked_cast<SysInt>(type)];
  }

  DynamicTriggerList* domainVal(DomainInt val) {
    D_ASSERT(val >= min && val <= max);
    return &_dynamicTriggers[checked_cast<SysInt>(val - min + 4)];
  }

  // It is important this object is never copied, but it can be moved
  TriggerObj(TriggerObj&&) = default;
  TriggerObj(const TriggerObj*) = delete;

};

class TriggerList {

  TriggerList(const TriggerList&);
  TriggerList();
  void operator=(const TriggerList&);
  bool onlyBounds;

public:
  TriggerList(bool _onlyBounds) : onlyBounds(_onlyBounds) {
  }

  vector<TriggerObj> dynTriggers;

  void addVariables(const std::vector<pair<DomainInt, DomainInt>>& doms) {
    SysInt old_varCount = dynTriggers.size();
    SysInt varCount_m = old_varCount + doms.size();
    dynTriggers.resize(varCount_m);
    for(int i = 0; i < doms.size(); ++i) {
      dynTriggers[old_varCount + i].min = doms[i].first;
      dynTriggers[old_varCount + i].max = doms[i].second;
      if(onlyBounds)
        dynTriggers[old_varCount + i]._dynamicTriggers.resize(4);
      else
        dynTriggers[old_varCount + i]._dynamicTriggers.resize(
            checked_cast<SysInt>(4 + (doms[i].second - doms[i].first + 1)));
    }
  }

  void dynamic_propagate(DomainInt _varNum, TrigType type, DomainInt domain_delta,
                         DomainInt val_removed = NoDomainValue) {
    const SysInt varNum = checked_cast<SysInt>(_varNum);
    D_ASSERT(val_removed == NoDomainValue ||
             (type == DomainRemoval && val_removed != NoDomainValue));
    D_ASSERT(!onlyBounds || type != DomainRemoval);
    DynamicTriggerList* trig;
    if(type != DomainRemoval) {
      trig = dynTriggers[varNum].trigger_type(type);
    } else {
      D_ASSERT(!onlyBounds);
      D_ASSERT(dynTriggers[varNum].min <= val_removed);
      D_ASSERT(dynTriggers[varNum].max >= val_removed);
      trig = dynTriggers[varNum].domainVal(val_removed);
    }

    // This is an optimisation, no need to push empty lists.
    if(!trig->empty())
      getQueue().pushDynamicTriggers(DynamicTriggerEvent(trig, checked_cast<SysInt>(domain_delta)));
  }

  void pushUpper(DomainInt varNum, DomainInt upper_delta) {
    D_ASSERT(upper_delta > 0 || getState().isFailed());
    dynamic_propagate(varNum, UpperBound, upper_delta);
  }

  void pushLower(DomainInt varNum, DomainInt lower_delta) {
    D_ASSERT(lower_delta > 0 || getState().isFailed());
    dynamic_propagate(varNum, LowerBound, lower_delta);
  }

  void push_assign(DomainInt varNum, DomainInt) {
    dynamic_propagate(varNum, Assigned, -1);
  }

  void pushDomainChanged(DomainInt varNum) {
    dynamic_propagate(varNum, DomainChanged, -1);
  }

  void pushDomain_removal(DomainInt varNum, DomainInt val_removed) {
    D_ASSERT(!onlyBounds);
    dynamic_propagate(varNum, DomainRemoval, -1, val_removed);
  }

  void addDynamicTrigger(DomainInt _b, Trig_ConRef t, TrigType type, DomainInt val,
                         TrigOp op = TO_Default) {
    const SysInt b = checked_cast<SysInt>(_b);
    D_ASSERT(!onlyBounds || type != DomainRemoval);
    D_ASSERT(t.con != NULL);

    DynamicTriggerList* queue;

    if(type != DomainRemoval) {
      queue = dynTriggers[b].trigger_type(type);
    } else {
      D_ASSERT(!onlyBounds);
      D_ASSERT(dynTriggers[b].min <= val);
      D_ASSERT(dynTriggers[b].max >= val);
      queue = dynTriggers[b].domainVal(val);
    }

    D_ASSERT(queue->sanityCheckList());
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
