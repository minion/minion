/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/
#include "minion.h"

namespace Controller
{
/// Lists all structures that must be locked before search.
// @todo This could be done more neatly... 

void lock(StateObj* stateObj)
{
  D_INFO(2, DI_SOLVER, "Starting Locking process");
  // do hackery involving the monotonic set.
  //getMemory(stateObj).monotonic_set=& getVars(stateObj).getBigRangevarContainer().bms_array;
  
  
  getVars(stateObj).getBigRangevarContainer().lock();
  
  getVars(stateObj).getSparseBoundvarContainer().lock();
  getVars(stateObj).getBooleanContainer().lock(); 
  getVars(stateObj).getBoundvarContainer().lock();
  
  int size = getState(stateObj).getConstraintList().size();
  for(int i = 0 ; i < size;i++)
	getState(stateObj).getConstraintList()[i]->setup();
  
  getMemory(stateObj).monotonicSet().lock(stateObj);
  getMemory(stateObj).backTrack().lock();
  getMemory(stateObj).nonBackTrack().lock();
  
  getTriggerMem(stateObj).finaliseTriggerLists();
  
  bool prop_to_do = true;
#ifdef USE_SETJMP
  int setjmp_return = SYSTEM_SETJMP(*(getState(stateObj).getJmpBufPtr()));
  if(setjmp_return != 0)
  {
	getState(stateObj).setFailed(true);
	getQueue(stateObj).clearQueues();
	return;
  }
#endif
  //while(prop_to_do)
  //{
	//prop_to_do = false;
	// We can't use incremental propagate until all constraints
	// have been setup, so this slightly messy loop is necessary
	// To propagate the first node.
    
    // No longer AC1, thank goodness.
	for(int i = 0; i < size; ++i)
	{
	  getState(stateObj).getConstraintList()[i]->full_propagate();
      getState(stateObj).getConstraintList()[i]->full_propagate_done=true;
	  if(getState(stateObj).isFailed()) 
		return;
	  // If queues not empty, more work to do.
	  if(!getQueue(stateObj).isQueuesEmpty())
	  {
          getQueue(stateObj).propagateQueueRoot();
          
          // old AC1 stuff
		//getQueue(stateObj).clearQueues();
		//prop_to_do = true;
	  }
	}
  //}
  
  getState(stateObj).markLocked();

} // lock()
}
