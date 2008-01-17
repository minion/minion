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


namespace Controller
{
  
  VARDEF(vector<TriggerRange> propagate_trigger_list);
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
	CON_INFO_ADDONE(AddSpecialToQueue);
    special_triggers.push_back(trigger);
  }
  
  inline void push_triggers(TriggerRange new_triggers)
  { 
	CON_INFO_ADDONE(AddConToQueue);
    D_INFO(1, DI_QUEUE, string("Adding new triggers. Trigger list size is ") + 
		   to_string(propagate_trigger_list.size()) + ".");
	propagate_trigger_list.push_back(new_triggers); 
  }
  
#ifdef DYNAMICTRIGGERS
  inline void push_dynamic_triggers(DynamicTrigger* new_dynamic_trig_range)
  { 
	CON_INFO_ADDONE(AddDynToQueue);
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
	BOOL* fail_ptr = &Controller::failed;
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
#ifdef NO_DYN_CHECK
		DynamicTrigger* next_queue_ptr;
#endif
		next_queue_ptr = it->next;
		D_INFO(1, DI_QUEUE, string("Will do ") + to_string(next_queue_ptr) + " next");
		CON_INFO_ADDONE(DynamicTrigger);
		it->propagate();  

		it = next_queue_ptr;
	  }
	}
	return false;
  }
  
  inline BOOL propagate_static_trigger_lists()
  {
	BOOL* fail_ptr = &Controller::failed;
	while(!propagate_trigger_list.empty())
	{
	  TriggerRange t = propagate_trigger_list.back();
	  int data_val = t.data;
	  propagate_trigger_list.pop_back();
	  
	  for(Trigger* it = t.begin(); it != t.end(); it++)
	  {
#ifndef USE_SETJMP
		if(*fail_ptr) 
		{
		  clear_queues();
		  return true; 
		}
#endif
		
#ifndef NO_DEBUG
		if(commandlineoption_fullpropagate)
		  it->full_propagate();
		else
		{
		  CON_INFO_ADDONE(StaticTrigger);
		  it->propagate(data_val);
		}
#else
		{
		  CON_INFO_ADDONE(StaticTrigger);
		  it->propagate(data_val);
		}
#endif
	  }
	}
	
	return false;
  }
  
  inline void propagate_queue()
  {
    D_INFO(2, DI_QUEUE, "Starting Propagation");
#ifdef USE_SETJMP
    int setjmp_return = SYSTEM_SETJMP(g_env);
	if(setjmp_return != 0)
	{ // Failure has occured
	  D_ASSERT(!Controller::failed);
	  Controller::failed = true;
	  clear_queues();
	  printf("!\n");
	  return;
	}
#endif
	
	while(true)
	{
#ifdef DYNAMICTRIGGERS
	  if (dynamic_triggers_used) 
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
	  CON_INFO_ADDONE(SpecialTrigger);
	  trig->special_check();

	} // while(true)
	
  } // end Function

  // This just allows SAC (which wants a list of vars)
  // and normal propagate to have the same input method.
  // Just checking the bounds doesn't make sense here, so we ignore it.
  template<typename Vars>
  inline void propagate_queue_vars(Vars& vars, bool /*CheckBounds*/)
  {	propagate_queue(); }
} // namespace Controller

