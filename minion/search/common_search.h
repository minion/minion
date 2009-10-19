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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef COMMON_SEARCH_H
#define COMMON_SEARCH_H

#include "../system/system.h"
#include "../solver.h"
#include "../variables/AnyVarRef.h"
#include "../variables/mappings/variable_neg.h"



namespace Controller
{
    
  
  /// Sets optimisation variable.
  template<typename VarRef>
  void optimise_maximise_var(StateObj* stateObj, VarRef var)
  {
      getOptions(stateObj).findAllSolutions();
      getState(stateObj).setOptimiseVar(new AnyVarRef(var));
      getState(stateObj).setOptimisationProblem(true);
  }
  
  /// Sets optimisation variable.
  template<typename VarRef>
  void optimise_minimise_var(StateObj* stateObj, VarRef var)
  {
      getOptions(stateObj).findAllSolutions();
      getState(stateObj).setOptimiseVar(new AnyVarRef(VarNeg<VarRef>(var)));
      getState(stateObj).setOptimisationProblem(true);
  }
  
  /// Ensures a particular constraint is satisfied by the solution.
  template<typename T>
  void check_constraint(StateObj* stateObj, T* con)
  {
      vector<AnyVarRef>& variables = *(con->get_vars_singleton());
      unsigned vec_size = variables.size();  
      
    DomainInt* values = (DomainInt*) alloca(vec_size * sizeof(DomainInt)); 
      //vector<DomainInt> values(vec_size);

      for(unsigned loop = 0; loop < vec_size; ++loop)
      {
        if(!variables[loop].isAssigned())
        {
          cerr << "Some variables are unassigned. Unless you purposefully " <<
          "left them out, have a look." << endl;
          return;
        }
        values[loop] = variables[loop].getAssignedValue();
      }
      
      if(!con->check_assignment(values, vec_size))
      {
        cerr << "A " << con->constraint_name() << " constraint is not satisfied by this sol!" << endl;
        cerr << "The constraint is over the following variables:" << endl;
        for(unsigned loop = 0; loop < vec_size; ++loop)
          cerr << variables[loop] << ",";
        cerr << endl;
        cerr << "Variables were assigned:" << endl;
        for(unsigned loop = 0; loop < vec_size; ++loop)
          cerr << values[loop] << ",";
        cerr << endl;
        cerr << "This is an internal bug. It shouldn't happen!!" << endl;
        cerr << "Please report this instance to the developers." << endl;
        FAIL_EXIT();
      }
  }

  /// All operations to be performed when a solution is found.
  /// This function checks the solution is correct, and prints it if required.
  inline void check_sol_is_correct(StateObj* stateObj)
  {
    if(getOptions(stateObj).solCallBack)
      getOptions(stateObj).solCallBack(stateObj);


    getState(stateObj).incrementSolutionCount();
    if(getOptions(stateObj).solsoutWrite)
    {
      vector<vector<AnyVarRef> > print_matrix = getState(stateObj).getPrintMatrix();
      for(unsigned i = 0; i < print_matrix.size(); ++i)
        for(unsigned j = 0; j < print_matrix[i].size(); ++j)
        {
          if(!print_matrix[i][j].isAssigned())
            INPUT_ERROR("Some variable was unassigned while writing solution to file.");
          solsoutFile << print_matrix[i][j].getAssignedValue() << " ";
        }
      solsoutFile << "\n";
    }
    
    if(getOptions(stateObj).print_solution)
    {
      vector<vector<AnyVarRef> > print_matrix = getState(stateObj).getPrintMatrix();  
      if(getOptions(stateObj).cspcomp)
      {
        cout << "v ";
        for(unsigned i = 0; i < print_matrix.size(); ++i)
          for(unsigned j = 0; j < print_matrix[i].size(); ++j)
          cout << print_matrix[i][j].getAssignedValue() << " ";
        cout << endl;
      }
      else if(!print_matrix.empty())
      {
        for(unsigned i = 0; i < print_matrix.size(); ++i)
        {
          if (!getOptions(stateObj).silent) cout << "Sol: ";  
          for(unsigned j = 0; j < print_matrix[i].size(); ++j)
          {
            if(!print_matrix[i][j].isAssigned())
              cout  << "[" << print_matrix[i][j].getMin() << "," << 
              print_matrix[i][j].getMax() << "]";
            else
              cout << print_matrix[i][j].getAssignedValue() << " ";
          }
          cout << endl;
        }
        if (!getOptions(stateObj).silent) cout << endl;
      }

    // TODO : Make this more easily changable.
      if (!getOptions(stateObj).silent) 
      {
        cout << "Solution Number: " << getState(stateObj).getSolutionCount() << endl;
        getState(stateObj).getOldTimer().printTimestepWithoutReset(Output_Always, "Time:");
        cout << "Nodes: " << getState(stateObj).getNodeCount() << endl << endl;
      }
    }

    if(!getOptions(stateObj).nocheck)
    {
      for(unsigned i = 0 ; i < getState(stateObj).getConstraintList().size();i++)
        check_constraint(stateObj, getState(stateObj).getConstraintList()[i]);
    }
  }

#include <fstream>

  //repeat declaration
  struct triple {
    bool isLeft;
    unsigned var;
    DomainInt val;
    
    triple(bool _isLeft, unsigned _var, DomainInt _val) : isLeft(_isLeft), var(_var), val(_val) {}
    friend std::ostream& operator<<(std::ostream& o, const triple& t)
    { o << "(" << t.isLeft << "," << t.var << "," << t.val << ")"; return o; }
  };

  template<typename VarArray, typename BranchList>
    inline void generateRestartFile(StateObj* stateObj, VarArray& var_array, BranchList& branches)
  {
    if(getOptions(stateObj).noresumefile) {
        return;
    }
    string filename = string("minion-resume-") + to_string(getpid());
    cout << "Output resume file to \"" << filename << "\"" << endl;
    ofstream fileout(filename.c_str());
    fileout << "MINION 3" << endl;
    fileout << "**CONSTRAINTS**" << endl;
    vector<triple> left_branches_so_far;
    left_branches_so_far.reserve(branches.size());
    for(vector<triple>::const_iterator curr = branches.begin(); curr != branches.end(); curr++) {
      if(curr->isLeft) {
        left_branches_so_far.push_back(*curr);
      } else {
        fileout << "watched-or({";
        for(vector<triple>::const_iterator lb = left_branches_so_far.begin();
            lb != left_branches_so_far.end();
            lb++) {
          fileout << "w-notliteral(";
          inputPrint(fileout, stateObj, var_array[lb->var].getBaseVar());
          fileout << "," << lb->val << "),";
        }
        fileout << "w-notliteral(";
        inputPrint(fileout, stateObj, var_array[curr->var].getBaseVar());
        fileout << "," << curr->val << ")})" << endl;
      }
    }
    fileout << "**EOF**" << endl;
  }
   
  /// Check if timelimit has been exceeded.
  template<typename VarArray, typename BranchList>
  inline bool do_checks(StateObj* stateObj, VarArray& var_array, BranchList& branches)
  {
    if(getState(stateObj).getNodeCount() == getOptions(stateObj).nodelimit) {
      generateRestartFile(stateObj, var_array, branches);
      return true;
    }
    
    if(getState(stateObj).isAlarmActivated())
    { // Either a timeout has occurred, or ctrl+c has been pressed.
      generateRestartFile(stateObj, var_array, branches);
      getState(stateObj).clearAlarm();
      if(getState(stateObj).isCtrlcPressed()) {
        return true;
      }

      if(getOptions(stateObj).cspcomp)
      {
        FAIL_EXIT("Time out");
      }
      
      getOptions(stateObj).printLine("Time out.");
      getTableOut().set("TimeOut", 1);
      return true;
    }
    return false;
  }
  

  template<typename T>
  void inline maybe_print_search_state(StateObj* stateObj, const char* name, T& vars)
  {
    if(getOptions(stateObj).dumptree)
      cout << name << getState(stateObj).getNodeCount() << "," << get_dom_as_string(vars) << endl;
  }

  void inline maybe_print_search_action(StateObj* stateObj, const char* action)
  {
      // used to print "bt" usually
      if(getOptions(stateObj).dumptree)
          cout << "SearchAction:" << action << endl;
  }

  void inline deal_with_solution(StateObj* stateObj)
  {
    // We have found a solution!
    check_sol_is_correct(stateObj);
    
    if(getState(stateObj).isOptimisationProblem())
    {
      if(!getState(stateObj).getOptimiseVar()->isAssigned())
      {
        cerr << "The optimisation variable isn't assigned at a solution node!" << endl;
        cerr << "Put it in the variable ordering?" << endl;
    cerr << "Aborting Search" << endl;
        exit(1);
      }
      
      cout << "Solution found with Value: " 
      << getState(stateObj).getOptimiseVar()->getAssignedValue() << endl;
      
      getState(stateObj).setOptimiseValue(getState(stateObj).getOptimiseVar()->getAssignedValue() + 1);         
    }
    // Note that sollimit = -1 if all solutions should be found.
    if(getState(stateObj).getSolutionCount() == getOptions(stateObj).sollimit)
      throw EndOfSearch();
  }

  void inline set_optimise_and_propagate_queue(StateObj* stateObj)
  {
    if(getState(stateObj).isOptimisationProblem())
      getState(stateObj).getOptimiseVar()->setMin(getState(stateObj).getOptimiseValue());
    getQueue(stateObj).propagateQueue();
  }

  void inline initalise_search(StateObj* stateObj)
  {
    getState(stateObj).setSolutionCount(0);  
    getState(stateObj).setNodeCount(0);
    
    if(!getOptions(stateObj).noTimers)
    {
        getState(stateObj).setupAlarm(getOptions(stateObj).time_limit, getOptions(stateObj).time_limit_is_CPU_time);
        getState(stateObj).setupCtrlc();
    }
    lock(stateObj);
    if (!getOptions(stateObj).silent) 
      getState(stateObj).getOldTimer().printTimestepWithoutReset(Output_1, "First Node Time: ");
    /// Failed initially propagating constraints!
    if(getState(stateObj).isFailed())
      return;
    if(getState(stateObj).isOptimisationProblem())
      getState(stateObj).setOptimiseValue(getState(stateObj).getOptimiseVar()->getMin()); 
  }
}

#endif
