/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: standard_queue.h 398 2006-10-17 09:49:19Z gentian $
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


namespace Controller
{
  
  VARDEF(vector<pair<Trigger, short> > propagate_trigger_list);
  
#ifdef DYNAMICTRIGGERS
  VARDEF(vector<DynamicTrigger*> dynamic_trigger_list);
#endif  
  
  // Special triggers are those which can only be run while the
  // normal queue is empty. This list is at the moment only used
  // by reified constraints when they want to start propagation.
  // I don't like it, but it is necesasary.
  VARDEF(vector<Constraint*> special_triggers);
  
  inline void push_special_trigger(Constraint* trigger)
  {
    special_triggers.push_back(trigger);
  }
  
  inline void push_triggers(TriggerRange new_triggers)
  { 
    D_INFO(1, DI_QUEUE, string("Adding ") + to_string(new_triggers.end() - new_triggers.begin())
		   + string(" new triggers. Trigger list size is ") + 
		   to_string(propagate_trigger_list.size()) + ".");
		   
    for(Trigger* it = new_triggers.begin(); it != new_triggers.end(); ++it)
      propagate_trigger_list.push_back(make_pair(*it, new_triggers.data));
  }
  
#ifdef DYNAMICTRIGGERS
  inline void push_dynamic_triggers(DynamicTrigger* new_dynamic_trig_range)
  { 
    D_ASSERT(new_dynamic_trig_range->sanity_check_list());
    dynamic_trigger_list.push_back(new_dynamic_trig_range);   
  }
#endif
  
  
  inline void clear_queues()
  {
	propagate_trigger_list.clear();
	dynamic_trigger_list.clear();
	
	if(!special_triggers.empty())
	{
	  int size = special_triggers.size();
	  for(int i = 0; i < size; ++i)
		special_triggers[i]->special_unlock();
	  special_triggers.clear();
	}
  }
  
  inline BOOL are_queues_empty()
  { 
	return propagate_trigger_list.empty() && dynamic_trigger_list.empty() &&
	       special_triggers.empty();
  }
  // next_queue_ptr is defined in constraint_dynamic.
  // It is used if pointers are moved around.
  
  
  inline BOOL propagate_dynamic_trigger_lists()
  {
	bool* fail_ptr = state->getFailedPtr();
	while(!dynamic_trigger_list.empty())
	{
	  DynamicTrigger* t = dynamic_trigger_list.back();
	  D_INFO(1, DI_QUEUE, string("Checking queue ") + to_string(t));
	  dynamic_trigger_list.pop_back();
	  DynamicTrigger* it = t->next;
	  
	  while(it != t)
	  {
#ifndef USE_SETJMP
		if(*fail_ptr) 
		{
		  clear_queues();
		  return true; 
		}
#endif
		D_INFO(1, DI_QUEUE, string("Checking ") + to_string(it));
		next_queue_ptr = it->next;
		D_INFO(1, DI_QUEUE, string("Will do ") + to_string(next_queue_ptr) + " next");
		it->propagate();  
		it = next_queue_ptr;
	  }
	}
	return false;
  }
  
  inline BOOL propagate_static_trigger_lists()
  {
	bool* fail_ptr = state->getFailedPtr();
	while(!propagate_trigger_list.empty())
	{
	  pair<Trigger,short> t = propagate_trigger_list.back();
 	  propagate_trigger_list.pop_back();
#ifndef USE_SETJMP
		if(*fail_ptr) 
		{
		  clear_queues();
		  return true; 
		}
#endif
		
#ifdef MORE_SEARCH_INFO
		if(options->fullpropagate)
		  t.first.full_propagate();
		else
		  t.first.propagate(t.second);
#else
		t.first.propagate(t.second);
#endif


	}
	
	return false;
  }
  
  inline void propagate_queue()
  {
    D_INFO(2, DI_QUEUE, "Starting Propagation");
#ifdef USE_SETJMP
    int setjmp_return = _setjmp(*(state->getJmpBufPtr()));
	if(setjmp_return != 0)
	{ // Failure has occured
	  D_ASSERT(!state->isFailed());
	  state->setFailed(true);
	  clear_queues();
	  return;
	}
#endif
	
	while(true)
	{
#ifdef DYNAMICTRIGGERS
	  if (state->isDynamicTriggersUsed()) 
	  {
		while(!propagate_trigger_list.empty() || !dynamic_trigger_list.empty())
		{
		  if(propagate_dynamic_trigger_lists())
			return;
		  
		  /* Don't like code duplication here but a slight efficiency gain */
		  if(propagate_static_trigger_lists())
			return;
		}
	  }
	  else
	  {
		if(propagate_static_trigger_lists())
		  return;
	  }
#else
	  if(propagate_static_trigger_lists())
		return;
#endif
	  
	  if(special_triggers.empty())
		return;
	  
	  D_INFO(1, DI_QUEUE, string("Doing a special trigger!"));
	  Constraint* trig = special_triggers.back();
	  special_triggers.pop_back();
	  trig->special_check();
	} // while(true)
  } // end Function

} // namespace Controller

