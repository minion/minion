/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/*
 *  constraint_checkassign.h
 *  cutecsp
 *
 *  Created by Chris Jefferson on 31/08/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

// This is a standard fall-back constraint which can be used when a
// constraint has not got a better method of implementing their
// inverse for reification.

#ifndef CONSTRAINT_CHECKASSIGN_H
#define CONSTRAINT_CHECKASSIGN_H

template<typename VarArray, typename OriginalConstraint>
struct CheckAssignConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "CheckAssign"; }
  
  OriginalConstraint& originalcon;
  VarArray variables;
  ReversibleInt assigned_vars;
  // To avoid allocating many vectors.
  vector<DomainInt> assignment;
  
  
  CheckAssignConstraint(StateObj* _stateObj, VarArray& vars, OriginalConstraint& con)
  : AbstractConstraint(_stateObj), originalcon(con),variables(vars), assigned_vars(stateObj), assignment(variables.size())
  { }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
	for(unsigned i = 0; i < variables.size(); ++i)
	  t.push_back(make_trigger(variables[i], Trigger(this, i), Assigned));
	return t;
  }
  
  virtual AbstractConstraint* reverse_constraint()
  { 
    cerr << "Check assign constraints shouldn't get reversed." << endl;
	FAIL_EXIT();
  }
  
  PROPAGATE_FUNCTION(int prop_val,DomainDelta delta)
  {
	PROP_INFO_ADDONE(CheckAssign);
    if(check_unsat(prop_val, delta))
	  getState(stateObj).setFailed(true);
  }
  
  virtual BOOL check_unsat(int,DomainDelta)
  {
  
	int count = assigned_vars;
	++count;
	int v_size = variables.size();
	D_ASSERT(count <= v_size);

	if(count == v_size)
	{
	  // XXX HACKY HACKY HACK
	  // OK, this is serious. The problem is that full_check_unsat
	  // might be called while other events are on the queue
	  // which end up in this function.
	  // This is a serious problem for all reified constraints.
	  // The following would fix it in this case:
	  // full_check_unsat()
	  // but that is too expensive in general.
	  for(int i = 0; i < v_size; ++i)
	  {
	    D_ASSERT(variables[i].isAssigned());
		assignment[i] = variables[i].getAssignedValue();
	  }
	  if(!check_assignment(&assignment.front(), assignment.size()))
	    return true;
	}
	assigned_vars = count; 
	return false;
  }
  
  virtual BOOL full_check_unsat()
  {
	unsigned counter = 0;
    for(unsigned i = 0; i < variables.size(); ++i)
	  if(variables[i].isAssigned()) ++counter;
	assigned_vars = counter;
	
	if(counter == variables.size())
	{
	  for(unsigned i = 0; i < variables.size(); ++i)
	  {
	    D_ASSERT(variables[i].isAssigned());
		assignment[i] = variables[i].getAssignedValue();
	  }
	  return !check_assignment(&assignment.front(), assignment.size());
	}
	return false;
  }
  
  virtual void full_propagate()
  {
    if(full_check_unsat())
	  getState(stateObj).setFailed(true);
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
	D_ASSERT(v_size == variables.size());
    return !originalcon.check_assignment(v, v_size);
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
	vector<AnyVarRef> vars;
	vars.reserve(variables.size());
	for(unsigned i = 0; i < variables.size(); ++i)
	  vars.push_back(variables[i]);
	return vars;
  }
  
  // Getting a satisfying assignment here is too hard
	 virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
   {
     MAKE_STACK_BOX(c, DomainInt, variables.size());
     
     for(int i = 0; i < variables.size(); ++i)
     {
       if(!variables[i].isAssigned()) 
       {  
         assignment.push_back(make_pair(i, variables[i].getMin()));
         assignment.push_back(make_pair(i, variables[i].getMax()));
         return true;
       }
       else
         c.push_back(variables[i].getAssignedValue());
     }
    
    if(check_assignment(c.begin(), c.size()))
    {  // Put the complete assignment in the box.
      for(int i = 0; i < variables.size(); ++i)
      {
        assignment.push_back(make_pair(i, c[i]));
      }
      return true;
    }
    return false;
   }
};

#endif
