/*
 *  test_functions.cpp
 *  cutecsp
 *
 *  Created by Chris Jefferson on 17/05/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */
#define NO_MAIN
#include "minion.h"

using Controller::print_matrix;
using Controller::test_solution;

void test_check_solution()
{
  vector<int> sol;
  BOOL match = true;
  for(unsigned i = 0; i < print_matrix[0].size(); ++i)
  {
	if(!print_matrix[0][i].isAssigned())
	{ 
	  cerr << "Test variable " << i << "not assigned!" << endl;
	  sol.push_back(-1);
	  match = false;
	}
	else
	{ sol.push_back(print_matrix[0][i].getAssignedValue()); }
  }
  
  if(sol != test_solution)
	match = false;
  
  if(!match)
  {
	cerr << "Test failed!" << endl;
	cerr << "Generated sol:";
	for(unsigned i = 0; i < sol.size(); ++i)
	  cerr << sol[i] << " ";
	cerr << endl;
	cerr << "From test case:";
	for(unsigned i = 0; i < test_solution.size(); ++i)
	  cerr << test_solution[i] << " ";
	cerr << endl;
	FAIL_EXIT();
  }
}


