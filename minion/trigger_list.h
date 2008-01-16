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

#ifndef TRIGGERLIST_H
#define TRIGGERLIST_H

class TriggerList;

namespace TriggerSpace
{
  VARDEF(vector<TriggerList*> trigger_lists);
  
  inline void addTriggerList(TriggerList* t) 
  { trigger_lists.push_back(t); }
  
  VARDEF(char* triggerlist_data);
  void finaliseTriggerLists();
}



class TriggerList
{
  TriggerList(const TriggerList&);
  void operator=(const TriggerList&);
  bool only_bounds;
  
public:
	TriggerList(bool _only_bounds = false) :
	only_bounds(_only_bounds)
  { 
	  var_count_m = 0;
	  lock_first = lock_second = 0; 
  }
  
  vector<vector<vector<Trigger> > > triggers;
  
#ifdef DYNAMICTRIGGERS
#ifdef WATCHEDLITERALS
  MemOffset dynamic_triggers;
#else
  BackTrackOffset dynamic_triggers;
#endif
#endif
  
  Trigger** trigger_data_m;
  
  int var_count_m;
  int lock_first;
  int lock_second;
  
  int vars_min_domain_val;
  int vars_max_domain_val;
  unsigned vars_domain_size;
  
  void lock(int size, int min_domain_val, int max_domain_val)
  {
	D_ASSERT(!lock_first && !lock_second);
	lock_first = true;
	var_count_m = size;
	vars_min_domain_val = min_domain_val;
	vars_max_domain_val = max_domain_val;
	vars_domain_size = max_domain_val - min_domain_val + 1;
	
	triggers.resize(4);
	for(unsigned i = 0; i < 4; ++i)
	  triggers[i].resize(var_count_m);
	
#ifdef DYNAMICTRIGGERS
	if(only_bounds)
  	  dynamic_triggers.request_bytes(size * sizeof(DynamicTrigger) * 4);  
	else
	  dynamic_triggers.request_bytes(size * sizeof(DynamicTrigger) * (4 + vars_domain_size));
#endif
	TriggerSpace::addTriggerList(this);
  }
  
  size_t memRequirement()
  {
	D_ASSERT(lock_first && !lock_second);
	size_t storage = 0;
	for(unsigned i = 0; i < 4; ++i)
	{
	  for(unsigned j = 0; j < triggers[i].size(); ++j)
	    storage += triggers[i][j].size();
	}
	return storage * sizeof(Trigger) + 4 * (var_count_m + 1) * sizeof(Trigger*);
  }
  
  void allocateMem(char* mem_start)
  {
	D_ASSERT(lock_first && !lock_second);
	lock_second = true;

#ifndef SLOW_TRIGGERS
	Trigger** trigger_ranges = (Trigger**)(mem_start);
	trigger_data_m = trigger_ranges;
	Trigger* trigger_data = (Trigger*)(mem_start + 4 * (triggers[UpperBound].size() + 1) * sizeof(Trigger*));
	
	for(unsigned int type = 0; type < 4; ++type)
	{
	  for(unsigned int i = 0; i < triggers[type].size(); ++i)
	  {
		*trigger_ranges = trigger_data;
		++trigger_ranges;
		for(unsigned int j = 0; j < triggers[type][i].size(); ++j)
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
#endif
	
#ifdef DYNAMICTRIGGERS
	DynamicTrigger* trigger_ptr = static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr());
	
	int trigger_types = ( only_bounds ? (4 + vars_domain_size) : 4 );
	for(unsigned i = 0; i < var_count_m * trigger_types; ++i)
	{
	  new (trigger_ptr + i) DynamicTrigger;
	  D_ASSERT((trigger_ptr + i)->sanity_check_list());
	}
#endif
  }
  
  pair<Trigger*, Trigger*> get_trigger_range(int var_num, TrigType type)
  {
    Trigger** first_trig = trigger_data_m + var_num + (var_count_m + 1) * type;
	Trigger* trig_range_start = *first_trig;
	first_trig++;
	Trigger* trig_range_end = *first_trig;
	return pair<Trigger*,Trigger*>(trig_range_start, trig_range_end);
  }
  
  void slow_trigger_push(int var_num, TrigType type, int delta)
  {
    if(!triggers[type][var_num].empty())
      Controller::push_triggers(TriggerRange(&triggers[type][var_num].front(),
      (&triggers[type][var_num].front()) + triggers[type][var_num].size(), delta));
  }
  
#ifdef DYNAMICTRIGGERS
  void dynamic_propogate(int var_num, TrigType type, int val_removed = -999)
  {
    D_ASSERT(val_removed == -999 || ( type == DomainRemoval && val_removed != -999) );
	DynamicTrigger* trig;
	if(type != DomainRemoval)
	{
	  trig = static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr())
			 + var_num + type*var_count_m;
	}
	else
	{
	  D_ASSERT(!only_bounds);
	  D_ASSERT(vars_min_domain_val <= val_removed);
	  D_ASSERT(vars_max_domain_val >= val_removed);
	  trig = static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr())
			 + var_num + (DomainRemoval + (val_removed - vars_min_domain_val)) * var_count_m;
	}
	// TODO: This shouldn't require next != NULL.
    if(trig->next != trig && trig->next != NULL)
	  Controller::push_dynamic_triggers(trig);
  }
#endif
  
  void push_upper(int var_num, int upper_delta)
  {
#ifdef DYNAMICTRIGGERS
    if (dynamic_triggers_used) dynamic_propogate(var_num, UpperBound);
#endif
	D_ASSERT(lock_second);
    D_ASSERT(upper_delta > 0 || Controller::failed);
	
#ifdef SLOW_TRIGGERS
	slow_trigger_push(var_num, UpperBound, upper_delta);
#else
    pair<Trigger*, Trigger*> range = get_trigger_range(var_num, UpperBound);
	if (range.first != range.second)
	  Controller::push_triggers(TriggerRange(range.first, range.second, upper_delta));
#endif	
  }
  
  void push_lower(int var_num, int lower_delta)
  { 
#ifdef DYNAMICTRIGGERS
    if (dynamic_triggers_used) dynamic_propogate(var_num, LowerBound);
#endif
	D_ASSERT(lock_second);
	D_ASSERT(lower_delta > 0 || Controller::failed);
#ifdef SLOW_TRIGGERS
	slow_trigger_push(var_num, LowerBound, lower_delta);
#else
	pair<Trigger*, Trigger*> range = get_trigger_range(var_num, LowerBound);
	if (range.first != range.second)
	  Controller::push_triggers(TriggerRange(range.first, range.second, lower_delta));
#endif
  }
  
  
  void push_assign(int var_num, int)
  { 
#ifdef DYNAMICTRIGGERS
    if (dynamic_triggers_used) dynamic_propogate(var_num, Assigned);
#endif
    D_ASSERT(lock_second);

#ifdef SLOW_TRIGGERS
	slow_trigger_push(var_num, Assigned, -1);
#else	
	pair<Trigger*, Trigger*> range = get_trigger_range(var_num, Assigned);
	if (range.first != range.second)
	  Controller::push_triggers(TriggerRange(range.first, range.second, -1));
#endif
  }
  
  void push_domain(int var_num)
  { 
#ifdef DYNAMICTRIGGERS
    if (dynamic_triggers_used) dynamic_propogate(var_num, DomainChanged);
#endif
	
#ifdef SLOW_TRIGGERS
	slow_trigger_push(var_num, DomainChanged, -1);
#else
	D_ASSERT(lock_second);
	pair<Trigger*, Trigger*> range = get_trigger_range(var_num, DomainChanged);
	if (range.first != range.second)	  
	  Controller::push_triggers(TriggerRange(range.first, range.second, -1)); 
#endif
  }
  
  void push_domain_removal(int var_num, int val_removed)
  { 
	D_ASSERT(!only_bounds);
#ifdef DYNAMICTRIGGERS
    dynamic_propogate(var_num, DomainRemoval, val_removed);
#endif
	D_ASSERT(lock_second);
  }
  
  void add_domain_trigger(int b, Trigger t)
  { 
	D_ASSERT(!only_bounds);
	D_ASSERT(lock_first && !lock_second); 
	triggers[DomainChanged][b].push_back(t); 
  }
  
  void add_trigger(int b, Trigger t, TrigType type)
  {
	D_ASSERT(type != DomainRemoval)
	D_ASSERT(lock_first && !lock_second);
	//if(type != DomainRemoval)
  	  triggers[type][b].push_back(t);
	//else
	//  triggers[type + val - vars_min_domain_val][b].push_back(t);
  }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(int b, DynamicTrigger* t, TrigType type, int val)
  {
    D_INFO(1, DI_QUEUE, "Adding Dynamic Trigger");
	D_ASSERT(lock_second);
	D_ASSERT(t->constraint != NULL);
	D_ASSERT(t->sanity_check == 1234);
	// This variable is only use in debug mode, and will be optimised away at any optimisation level.
	DynamicTrigger* old_list;
	old_list = t->next;
	DynamicTrigger* queue;
	if(type != DomainRemoval)
	{
	  queue = static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr())
			  + b + type*var_count_m;
	}
	else
	{
	  D_ASSERT(!only_bounds);
	  D_ASSERT(vars_min_domain_val <= val);
	  D_ASSERT(vars_max_domain_val >= val);
	  queue = static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr())
			  + b + (DomainRemoval + (val - vars_min_domain_val)) * var_count_m;
	}
	D_ASSERT(queue->sanity_check_list());
	t->add_after(queue);
	D_ASSERT(old_list == NULL || old_list->sanity_check_list(false));
  }
#endif
  
};

namespace TriggerSpace
{
  inline void finaliseTriggerLists()
  {
	size_t trigger_size = 0;
	for(unsigned int i=0;i<trigger_lists.size();i++)
	  trigger_size += trigger_lists[i]->memRequirement();
	triggerlist_data = new char[trigger_size];
	
	char* triggerlist_offset = triggerlist_data;
	for(unsigned int i=0;i<trigger_lists.size();i++)
	{
	  size_t offset = trigger_lists[i]->memRequirement();
	  trigger_lists[i]->allocateMem(triggerlist_offset);
	  triggerlist_offset += offset;
	}
	D_ASSERT(triggerlist_offset - triggerlist_data == (int)trigger_size);
  }
}

#endif //TRIGGERLIST_H

