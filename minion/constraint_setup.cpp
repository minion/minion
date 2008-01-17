/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/
#define NO_MAIN

#include "minion.h"

namespace Controller
{
/// Lists all structures that must be locked before search.
// @todo This could be done more neatly... 

void lock()
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
#ifdef USE_SETJMP
  int setjmp_return = SYSTEM_SETJMP(g_env);
  if(setjmp_return != 0)
  {
	Controller::failed = true;
	clear_queues();
	return;
  }
#endif
  while(prop_to_do)
  {
	prop_to_do = false;
	// We can't use incremental propagate until all constraints
	// have been setup, so this slightly messy loop is necessary
	// To propagate the first node.
	for(int i = 0; i < size; ++i)
	{
	  constraints[i]->full_propagate();
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
	dynamic_constraints[i]->full_propagate();
	propagate_queue();
	if(Controller::failed) 
	  return;
  }
#endif
} // lock()
}
