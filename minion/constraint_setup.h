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
  
/// Setups up and does a first propagation of the constraints
inline void setup_constraints()
{

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
  
  size_t size = constraints.size();
  for(size_t i = 0 ; i < size;i++)
	constraints[i]->setup();
  
  TriggerSpace::finaliseTriggerLists();
  
  backtrackable_memory.final_lock();
  memory_block.final_lock();  
  
  while(true)
  {
	// We can't use incremental propagate until all constraints
	// have been setup, so this slightly messy loop is necessary
	// To propagate the first node.
	for(size_t i = 0; i < size; ++i)
	  constraints[i]->full_propogate();

#ifdef DYNAMICTRIGGERS
	for(int i = 0; i < dynamic_size; ++i)
	  dynamic_constraints[i]->full_propogate();
#endif
	
	if(are_queues_empty())
	  return;
	clear_queues();
  }
}
}

