/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/*
 *  constraint_setup.h
 *  cutecsp
 *
 *  Created by Chris Jefferson on 17/05/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

namespace Controller
{
  
  inline void add_constraint(Constraint* c)
{ constraints.push_back(c); }
  
#ifdef DYNAMICTRIGGERS
  inline void add_constraint(DynamicConstraint* c)
{ dynamic_constraints.push_back(c); }
#endif
  

  
/// This is a special limited version of the normal propagation,
/// that takes a list of constraints that are allowed to be propagated
/// at present.
inline bool special_limit_propogate_static_queue(const set<Constraint*>& constraints)
{
	BOOL* fail_ptr = &Controller::failed;
	while(!propogate_trigger_list.empty())
	{
	  TriggerRange t = propogate_trigger_list.back();
	  short data_val = t.data;
	  propogate_trigger_list.pop_back();
	  
	  for(Trigger* it = t.start; it != t.end ; it++)
	  {
#ifndef USE_SETJMP
		if(*fail_ptr) 
		{
		  clear_queues();
		  return true; 
		}
#endif
		
		it->propogate(data_val);		
	  }
	}
	
	return false;
}

/// Lists all structures that must be locked before search.
// @todo This could be done more neatly... 
inline void lock()
{
  D_INFO(2, DI_SOLVER, "Starting Locking process");
  rangevar_container.lock();
  big_rangevar_container.lock();
  sparse_boundvar_container.lock();
  boolean_container.lock(); 
  boundvar_container.lock();
#ifdef DYNAMICTRIGGERS
  int dynamic_size = dynamic_constraints.size();
  for(int i = 0; i < dynamic_size; ++i)
	dynamic_constraints[i]->setup();
#endif
  backtrackable_memory.lock();
  memory_block.lock();
  atexit(Controller::finish);
  
  int size = constraints.size();
  for(int i = 0 ; i < size;i++)
	constraints[i]->setup();
  
  TriggerSpace::finaliseTriggerLists();
  
  backtrackable_memory.final_lock();
  memory_block.final_lock();  
  
  bool prop_to_do = true;
  while(prop_to_do)
  {
	prop_to_do = false;
	// We can't use incremental propagate until all constraints
	// have been setup, so this slightly messy loop is necessary
	// To propagate the first node.
	for(int i = 0; i < size; ++i)
	{
	  constraints[i]->full_propogate();
	  if(Controller::failed) 
		return;
	  // If queues not empty, more work to do.
	  if(!are_queues_empty())
	  {
		clear_queues();
		prop_to_do = true;
	  }
	}
  }
  
#ifdef DYNAMICTRIGGERS
  for(int i = 0; i < dynamic_size; ++i)
  {
	dynamic_constraints[i]->full_propogate();
	propogate_queue();
	if(Controller::failed) 
	  return;
  }
#endif
} // lock()

} // namespace Controller

