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

#ifndef TRIGGERLIST_H
#define TRIGGERLIST_H

#include "system/system.h"
#include "solver.h"
#include "constraints/triggers.h"
#include "constraints/constraint_abstract.h"

#include "memory_management/backtrackable_memory.h"
#include "memory_management/nonbacktrack_memory.h"

class TriggerList;

class TriggerMem
{
  vector<TriggerList*> trigger_lists;
  char* triggerlist_data;
  StateObj* stateObj;

public:
  void addTriggerList(TriggerList* t)
  { trigger_lists.push_back(t); }

  void finaliseTriggerLists();

  TriggerMem(StateObj* _stateObj) : triggerlist_data(NULL), stateObj(_stateObj)
  {}

  void allocateTriggerListData(UnsignedSysInt mem)
  {
    D_ASSERT(triggerlist_data == NULL);
    triggerlist_data = new char[mem];
  }

  char* getTriggerListDataPtr() { return triggerlist_data; }
  ~TriggerMem()
  { delete[] triggerlist_data; }
};

class TriggerList
{
  StateObj* stateObj;

  TriggerList(const TriggerList&);
  TriggerList();
  void operator=(const TriggerList&);
  bool only_bounds;

public:
  TriggerList(StateObj* _stateObj, bool _only_bounds) : stateObj(_stateObj),
  only_bounds(_only_bounds)
  {
    var_count_m = 0;
    lock_first = lock_second = 0;

    dynamic_triggers = 0;
    trigger_data_m = 0;
    vars_min_domain_val = 0;
    vars_max_domain_val = 0;
    vars_domain_size = 0;
  }

  vector<vector<vector<Trigger> > > triggers;

  void* dynamic_triggers;


  Trigger** trigger_data_m;

  SysInt var_count_m;
  SysInt lock_first;
  SysInt lock_second;

  DomainInt vars_min_domain_val;
  DomainInt vars_max_domain_val;
  UnsignedSysInt vars_domain_size;

  void lock(SysInt size, DomainInt min_domain_val, DomainInt max_domain_val)
  {
    D_ASSERT(!lock_first && !lock_second);
    lock_first = true;
    var_count_m = size;
    vars_min_domain_val = min_domain_val;
    vars_max_domain_val = max_domain_val;
    vars_domain_size = checked_cast<UnsignedSysInt>(max_domain_val - min_domain_val + 1);

    triggers.resize(4);
    for(UnsignedSysInt i = 0; i < 4; ++i)
      triggers[i].resize(var_count_m);

#ifdef DYNAMICTRIGGERS
    if(only_bounds)
      dynamic_triggers = checked_malloc(size * sizeof(DynamicTrigger) * 4);
    else
      dynamic_triggers = checked_malloc(size * sizeof(DynamicTrigger) * (4 + vars_domain_size));
#else
    if(only_bounds)
      dynamic_triggers = getMemory(stateObj).backTrack().request_bytes(size * sizeof(DynamicTrigger) * 4);
    else
      dynamic_triggers = getMemory(stateObj).backTrack().request_bytes(size * sizeof(DynamicTrigger) * (4 + vars_domain_size));
#endif
    getTriggerMem(stateObj).addTriggerList(this);
  }

  size_t memRequirement()
  {
    D_ASSERT(lock_first && !lock_second);
    size_t storage = 0;
    for(UnsignedSysInt i = 0; i < 4; ++i)
    {
      for(UnsignedSysInt j = 0; j < triggers[i].size(); ++j)
        storage += triggers[i][j].size();
    }
    return storage * sizeof(Trigger) + 4 * (var_count_m + 1) * sizeof(Trigger*);
  }

  struct CompareMem
  {
    bool operator()(const Trigger& t1, const Trigger& t2)
    { return t1.constraint->getTrigWeight() < t2.constraint->getTrigWeight(); }
  };

  void allocateMem(char* mem_start)
  {
#ifdef SORT_TRIGGERRANGES
    // We can sort triggers if you like here!
    for(UnsignedSysInt type = 0; type < 4; ++type)
    {
      for(UnsignedSysInt i = 0; i < triggers[type].size(); ++i)
      {
        std::sort(triggers[type][i].begin(), triggers[type][i].end(), CompareMem());
      }
    }
#endif

    D_ASSERT(lock_first && !lock_second);
    lock_second = true;
    Trigger** trigger_ranges = (Trigger**)(mem_start);
    trigger_data_m = trigger_ranges;
    Trigger* trigger_data = (Trigger*)(mem_start + 4 * (triggers[UpperBound].size() + 1) * sizeof(Trigger*));

    for(UnsignedSysInt type = 0; type < 4; ++type)
    {
      for(UnsignedSysInt i = 0; i < triggers[type].size(); ++i)
      {
        *trigger_ranges = trigger_data;
        ++trigger_ranges;
        for(UnsignedSysInt j = 0; j < triggers[type][i].size(); ++j)
        {
          *trigger_data = triggers[type][i][j];
          trigger_data++;
        }
      }
      *trigger_ranges = trigger_data;
      ++trigger_ranges;
    }

    D_ASSERT(static_cast<void*>(mem_start + 4 * (var_count_m + 1) * sizeof(Trigger*)) ==
      static_cast<void*>(trigger_ranges));


  // This is a common C++ trick to completely free the memory of an object.
    {
      vector<vector<vector<Trigger> > > t;
      triggers.swap(t);
    }

    DynamicTrigger* trigger_ptr = static_cast<DynamicTrigger*>(dynamic_triggers);

    DomainInt trigger_types = ( only_bounds ? 4 : (4 + vars_domain_size));
    for(UnsignedSysInt i = 0; i < var_count_m * trigger_types; ++i)
    {
      new (trigger_ptr + i) DynamicTrigger;
      D_ASSERT((trigger_ptr + i)->sanity_check_list());
    }
  }

  pair<Trigger*, Trigger*> get_trigger_range(DomainInt var_num, TrigType type)
  {
    Trigger** first_trig = trigger_data_m + checked_cast<SysInt>(var_num) + (var_count_m + 1) * type;
    Trigger* trig_range_start = *first_trig;
    first_trig++;
    Trigger* trig_range_end = *first_trig;
    return pair<Trigger*,Trigger*>(trig_range_start, trig_range_end);
  }

  void dynamic_propagate(DomainInt var_num, TrigType type, DomainInt val_removed = NoDomainValue)
  {
    D_ASSERT(val_removed == NoDomainValue || ( type == DomainRemoval && val_removed != NoDomainValue) );
    D_ASSERT(!only_bounds || type != DomainRemoval);
    DynamicTrigger* trig;
    if(type != DomainRemoval)
    {
      trig = static_cast<DynamicTrigger*>(dynamic_triggers)
        + checked_cast<SysInt>(var_num + type*var_count_m);
    }
    else
    {
      D_ASSERT(!only_bounds);
      D_ASSERT(vars_min_domain_val <= val_removed);
      D_ASSERT(vars_max_domain_val >= val_removed);
      trig = static_cast<DynamicTrigger*>(dynamic_triggers)
        + checked_cast<SysInt>(var_num + (DomainRemoval + (val_removed - vars_min_domain_val)) * var_count_m);
    }
    D_ASSERT(trig->next != NULL);
    // This is an optimisation, no need to push empty lists.
    if(trig->next != trig)
      getQueue(stateObj).pushDynamicTriggers(trig);
  }

  void push_upper(DomainInt var_num, DomainInt upper_delta)
  {
    if (getState(stateObj).isDynamicTriggersUsed()) dynamic_propagate(var_num, UpperBound);
    D_ASSERT(lock_second);
    D_ASSERT(upper_delta > 0 || getState(stateObj).isFailed());

    pair<Trigger*, Trigger*> range = get_trigger_range(var_num, UpperBound);
    if(range.first != range.second)
      getQueue(stateObj).pushTriggers(TriggerRange(range.first, range.second,
                                                   checked_cast<SysInt>(upper_delta)));
  }

  void push_lower(DomainInt var_num, DomainInt lower_delta)
  {
    if (getState(stateObj).isDynamicTriggersUsed()) dynamic_propagate(var_num, LowerBound);
    D_ASSERT(lock_second);
    D_ASSERT(lower_delta > 0 || getState(stateObj).isFailed());
    pair<Trigger*, Trigger*> range = get_trigger_range(var_num, LowerBound);
    if(range.first != range.second)
      getQueue(stateObj).pushTriggers(TriggerRange(range.first, range.second,
                                                   checked_cast<SysInt>(lower_delta)));
  }


  void push_assign(DomainInt var_num, DomainInt)
  {
    if (getState(stateObj).isDynamicTriggersUsed()) dynamic_propagate(var_num, Assigned);
    D_ASSERT(lock_second);
    pair<Trigger*, Trigger*> range = get_trigger_range(var_num, Assigned);
    if(range.first != range.second)
      getQueue(stateObj).pushTriggers(TriggerRange(range.first, range.second, -1));
  }

  void push_domain_changed(DomainInt var_num)
  {
    if (getState(stateObj).isDynamicTriggersUsed()) dynamic_propagate(var_num, DomainChanged);

    D_ASSERT(lock_second);
    pair<Trigger*, Trigger*> range = get_trigger_range(var_num, DomainChanged);
    if (range.first != range.second)
      getQueue(stateObj).pushTriggers(TriggerRange(range.first, range.second, -1));
  }

  void push_domain_removal(DomainInt var_num, DomainInt val_removed)
  {
    D_ASSERT(!only_bounds);
    dynamic_propagate(var_num, DomainRemoval, val_removed);
    D_ASSERT(lock_second);
  }

  void add_domain_trigger(DomainInt b, Trigger t)
  {
    D_ASSERT(!only_bounds);
    D_ASSERT(lock_first && !lock_second);
    triggers[DomainChanged][checked_cast<SysInt>(b)].push_back(t);
  }

  void add_trigger(DomainInt b, Trigger t, TrigType type)
  {
    D_ASSERT(type != DomainRemoval);
    D_ASSERT(lock_first && !lock_second);
    triggers[type][checked_cast<SysInt>(b)].push_back(t);
  }


  void addDynamicTrigger(DomainInt b, DynamicTrigger* t, TrigType type, DomainInt val BT_FUNDEF)
  {
    D_ASSERT(lock_second);
    D_ASSERT(!only_bounds || type != DomainRemoval);
    D_ASSERT(t->constraint != NULL);
    D_ASSERT(t->sanity_check == 1234);
  // This variable is only use in debug mode, and will be optimised away at any optimisation level.
    DynamicTrigger* old_list;
    (void)old_list;
    old_list = t->next;
    DynamicTrigger* queue;
    if(type != DomainRemoval)
    {
      queue = static_cast<DynamicTrigger*>(dynamic_triggers)
        + checked_cast<SysInt>(b + type*var_count_m);
    }
    else
    {
      D_ASSERT(!only_bounds);
      D_ASSERT(vars_min_domain_val <= val);
      D_ASSERT(vars_max_domain_val >= val);
      queue = static_cast<DynamicTrigger*>(dynamic_triggers)
        + checked_cast<SysInt>(b + (DomainRemoval + (val - vars_min_domain_val)) * var_count_m);
    }
    D_ASSERT(queue->sanity_check_list());

#ifdef BTWLDEF
    switch(op)
    {
        case TO_Default:
            D_DATA(t->setQueue((DynamicTrigger*)BAD_POINTER));
        break;
        case TO_Store:
        t->setQueue(queue);
        break;
        case TO_Backtrack:
            D_ASSERT(t->getQueue() != (DynamicTrigger*)BAD_POINTER);
            getQueue(stateObj).getTbq().addTrigger(t);
            // Add to queue.
            t->setQueue(queue);
        break;
        default:
        abort();
    }
#endif

    t->add_after(queue);
    D_ASSERT(old_list == NULL || old_list->sanity_check_list(false));
  }

};

void inline TriggerMem::finaliseTriggerLists()
  {
    size_t trigger_size = 0;
    for(UnsignedSysInt i = 0;i < trigger_lists.size(); i++)
      trigger_size += trigger_lists[i]->memRequirement();
    getTriggerMem(stateObj).allocateTriggerListData(trigger_size);

    char* triggerlist_offset = getTriggerMem(stateObj).getTriggerListDataPtr();

    for(UnsignedSysInt i=0;i<trigger_lists.size();i++)
    {
      size_t offset = trigger_lists[i]->memRequirement();
      trigger_lists[i]->allocateMem(triggerlist_offset);
      triggerlist_offset += offset;
    }
    D_ASSERT(triggerlist_offset - getTriggerMem(stateObj).getTriggerListDataPtr() == (SysInt)trigger_size);
  }

inline void releaseTrigger(StateObj* stateObj, DynamicTrigger* t BT_FUNDEF_NODEFAULT)
{
#ifdef BTWLDEF
    switch(op)
    {
        case TO_Default:
            D_DATA(t->setQueue((DynamicTrigger*)BAD_POINTER));
        break;
        case TO_Store:
        t->setQueue((DynamicTrigger*)(NULL));
        break;
        case TO_Backtrack:
            D_ASSERT(t->getQueue() != (DynamicTrigger*)BAD_POINTER);
            getQueue(stateObj).getTbq().addTrigger(t);
            // Add to queue.
            t->setQueue((DynamicTrigger*)(NULL));
        break;
        default:
        abort();
    }
#endif

    t->remove();
}

 inline void attachTriggerToNullList(StateObj* stateObj, DynamicTrigger* t BT_FUNDEF_NODEFAULT)
 {    static DynamicTrigger dt;
    DynamicTrigger* queue = &dt;

#ifdef BTWLDEF
    switch(op)
    {
        case TO_Default:
            D_DATA(t->setQueue((DynamicTrigger*)BAD_POINTER));
        break;
        case TO_Store:
        t->setQueue(queue);
        break;
        case TO_Backtrack:
            D_ASSERT(t->getQueue() != (DynamicTrigger*)BAD_POINTER);
            getQueue(stateObj).getTbq().addTrigger(t);
            // Add to queue.
            t->setQueue(queue);
        break;
        default:
        abort();
    }
#endif
    t->add_after(queue);
 }


#endif //TRIGGERLIST_H
