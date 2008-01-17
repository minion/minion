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
  varContainer.getRangevarContainer().lock();
  varContainer.getBigRangevarContainer().lock();
  varContainer.getSparseBoundvarContainer().lock();
  varContainer.getBooleanContainer().lock(); 
  varContainer.getBoundvarContainer().lock();
#ifdef DYNAMICTRIGGERS
  int dynamic_size = state.getDynamicConstraintList().size();
  for(int i = 0; i < dynamic_size; ++i)
	state.getDynamicConstraintList()[i]->setup();
#endif
  backtrackable_memory.lock();
  memory_block.lock();
  atexit(Controller::finish);
  
  int size = state.getConstraintList().size();
  for(int i = 0 ; i < size;i++)
	state.getConstraintList()[i]->setup();
  
  triggerMem->finaliseTriggerLists();
  
  backtrackable_memory.final_lock();
  memory_block.final_lock();  
  
  bool prop_to_do = true;
#ifdef USE_SETJMP
  int setjmp_return = SYSTEM_SETJMP(*(state.getJmpBufPtr()));
  if(setjmp_return != 0)
  {
	state.setFailed(true);
	queues.clearQueues();
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
	  state.getConstraintList()[i]->full_propagate();
	  if(state.isFailed()) 
		return;
	  // If queues not empty, more work to do.
	  if(!queues.isQueuesEmpty())
	  {
		queues.clearQueues();
		prop_to_do = true;
	  }
	}
  }
  
#ifdef DYNAMICTRIGGERS
  for(int i = 0; i < dynamic_size; ++i)
  {
	state.getDynamicConstraintList()[i]->full_propagate();
	queues.propagateQueue();
	if(state.isFailed()) 
	  return;
  }
#endif
} // lock()
}
