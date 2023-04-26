/*
 * Minion http://minion.sourceforge.net
 * Copyright (C) 2006-09
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#ifndef TABSEARCHMANAGER_H
#define TABSEARCHMANAGER_H

#include "../preprocess.h"
#include "common_search.h"
#include "variable_orderings.h"

namespace Controller {

//  Search for -X-tabulation option. 
//  Just replace the search method of StandardSearchManager

struct TabSearchManager : public StandardSearchManager {
  using StandardSearchManager::StandardSearchManager;
  
  //  -X-tabulation extra data. 
  vector<vector<DomainInt>> tab_mapping;
  vector<DomainInt> min_domain;
  vector<DomainInt> num_vals;
  
  //  Part of -X-tabulation
  double assignmentNumber(vector<DomainInt> assignment) {
    //  Convert an assignment into a number, interpreting each variable assignment as one digit (0..d-1). 
    std::cout << "in assignmentNumber: " << assignment << std::endl;
    double tmp=tab_mapping[0][assignment[0]-min_domain[0]];
    assert (tmp>-1);
    for(int i=1; i<assignment.size(); i++) {
      tmp = tmp*num_vals[i];  //  Scale up by the base of the current digit.
      double current_digit=tab_mapping[i][assignment[i]-min_domain[i]];
      assert (current_digit>-1);
      tmp = tmp+current_digit;  //  Add the 0-based value of the current digit. 
    }
    return tmp;
  }

  // Most basic search procedure
  virtual void search() {
    if(getOptions().sollimit>0) {
      getOptions().sollimit=getOptions().sollimit+2;   //  Hack to stop Minion's sol limit triggering, so I can catch that case in here. 
    }
    
    maybe_print_node();
    
    //  added for tabulation mode.
    int n_nonaux=varArray.size();
    if(varOrder->hasAuxVars()) {
      n_nonaux=varOrder->auxVarStart();
    }
    
    //  Initialise the domain map (from each domain value to 0..d-1 range
    min_domain.resize(n_nonaux);
    tab_mapping.resize(n_nonaux);
    num_vals.resize(n_nonaux);
    
    for(int i=0; i<n_nonaux; i++) {
      min_domain[i]=varArray[i].min();
      int n=0;  //  will be number of values at the end
      
      for(DomainInt j=varArray[i].min(); j<=varArray[i].max(); j++) {
        if(varArray[i].inDomain(j)) {
          tab_mapping[i].push_back(n);
          n++;
        }
        else {
          tab_mapping[i].push_back(-1);
        }
      }
      
      num_vals[i]=n;
    }
    
    long fails=0;
    
    while(true) {
      D_ASSERT(getQueue().isQueuesEmpty());
      
      getState().incrementNodeCount();
      
      if(!getState().isFailed()) {
        //  Progress check for tabulation mode.
        long nodecount=getState().getNodeCount();
        if(nodecount==1000 || (nodecount%10000)==0) {
            //  Special checks when doing tabulation (-X-tabulation flag) -- has enough progress been made so far.
            //  Read the current assignment. 
            
            vector<DomainInt> cur_assign;
            for(int i=0; i<n_nonaux; i++) {
              cur_assign.push_back(varArray[i].min());
            }
            double an=assignmentNumber(cur_assign);
            
            vector<DomainInt> final_assign;
            for(int i=0; i<n_nonaux; i++) {
              final_assign.push_back(tab_mapping[i].size()+min_domain[i]-1);
            }
            double final_an=assignmentNumber(final_assign);
            
            double prop=(an*(getOptions().nodelimit))/(final_an*(getState().getNodeCount()));
            std::cout << an << " final: " << final_an << " nodelim:" << getOptions().nodelimit <<  "  prop: "<< prop <<std::endl;
            if(prop<1.0) {
              //  Bail out with a message to SR, stopped by failed progress check. 
              std::cout <<  "STOP-PC" << std::endl;
              return;
            }
        }
      }
      
      // Doesn't do usual checks from StandardSearch
      //check_func(varArray, branches);
      
      if(getState().getNodeCount()>getOptions().nodelimit) {
        std::cout << "STOP-NC" << std::endl;
        return;
      }
      if(getOptions().sollimit>0 && getState().getSolutionCount()>getOptions().sollimit-2) {
        std::cout << "STOP-SC" << std::endl;
        return;
      }
      
      pair<SysInt, DomainInt> varval = varOrder->pickVarVal();

      if(varval.first == -1) {
        // We have found a solution!
        check_sol_is_correct();
        maybe_print_node(true);
        handle_sol_func();
        if(varOrder->hasAuxVars()) { // There are AUX vars at the end of the var ordering.
          // Backtrack out of them.
          jump_out_aux_vars();
        }

        // If we are not finished, then go into the loop below.
        getState().setFailed(true);
      } else {
        maybe_print_node();
        branch_left(varval);
        prop->prop(varArray);
        if(getState().isFailed()) {
          fails++;
        }
      }
      
      // loop to
      while(getState().isFailed()) {
        if(branches.size() == 0) {
          return;
        }

        maybe_print_backtrack();
        bool flag = branch_right();
        if(!flag) { // No remaining left branches to branch right.
          //std::cout << "prop fails:" << fails <<std::endl;
          if(fails==0) {
            std::cout << "STOP-BTFREE" << std::endl;
          }
          return;
        }
        getState().incrementBacktrackCount();
        prop->prop(varArray);
        if(getState().isFailed()) {
          fails++;
        }
      }
    }
  }
};
} // namespace Controller

#endif
