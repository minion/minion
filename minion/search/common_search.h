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
      getState(stateObj).setRawOptimiseVar(new AnyVarRef(var));
      getState(stateObj).setMaximise(true);

  }
  
  /// Sets optimisation variable.
  template<typename VarRef>
  void optimise_minimise_var(StateObj* stateObj, VarRef var)
  {
      getOptions(stateObj).findAllSolutions();
      getState(stateObj).setOptimiseVar(new AnyVarRef(VarNeg<VarRef>(var)));
      getState(stateObj).setOptimisationProblem(true);
      getState(stateObj).setRawOptimiseVar(new AnyVarRef(var));
      getState(stateObj).setMaximise(false);
  }
  
  /// Ensures a particular constraint is satisfied by the solution.
  template<typename T>
  void check_constraint(StateObj* stateObj, T* con)
  {
      vector<AnyVarRef>& variables = *(con->get_vars_singleton());
      UnsignedSysInt vec_size = variables.size();  
      
    DomainInt* values = (DomainInt*) alloca(vec_size * sizeof(DomainInt)); 
      //vector<DomainInt> values(vec_size);

      for(UnsignedSysInt loop = 0; loop < vec_size; ++loop)
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
        cerr << "A " << con->extended_name() << " constraint is not satisfied by this sol!" << endl;
        cerr << "The constraint is over the following variables:" << endl;
        for(UnsignedSysInt loop = 0; loop < vec_size; ++loop)
          cerr << variables[loop] << ",";
        cerr << endl;
        cerr << "Variables were assigned:" << endl;
        for(UnsignedSysInt loop = 0; loop < vec_size; ++loop)
          cerr << values[loop] << ",";
        cerr << endl;
        cerr << "This is an internal bug. It shouldn't happen!!" << endl;
        cerr << "Please report this instance to the developers." << endl;
        FAIL_EXIT();
      }
  }

  template<typename Stream, typename PrintMatrix>
  void print_solution(StateObj* stateObj, Stream& sout, const PrintMatrix& print_matrix)
  {
      if(getOptions(stateObj).cspcomp)
      {
        sout << "v ";
        for(UnsignedSysInt i = 0; i < print_matrix.size(); ++i)
          for(UnsignedSysInt j = 0; j < print_matrix[i].size(); ++j)
          sout << print_matrix[i][j].getAssignedValue() << " ";
        sout << endl;
      }
      else if(!print_matrix.empty())
      {
        for(UnsignedSysInt i = 0; i < print_matrix.size(); ++i)
        {
          if (!getOptions(stateObj).silent) sout << "Sol: ";  
          for(UnsignedSysInt j = 0; j < print_matrix[i].size(); ++j)
          {
            if(!print_matrix[i][j].isAssigned())
              sout  << "[" << print_matrix[i][j].getMin() << "," << 
              print_matrix[i][j].getMax() << "]";
            else
              sout << print_matrix[i][j].getAssignedValue() << " ";
          }
          sout << endl;
        }
        if (!getOptions(stateObj).silent) sout << endl;
      }

    // TODO : Make this more easily changable.
      if (!getOptions(stateObj).silent) 
      {
        sout << "Solution Number: " << getState(stateObj).getSolutionCount() << endl;
        getState(stateObj).getOldTimer().printTimestepWithoutReset(sout, Output_Always, "Time:");
        sout << "Nodes: " << getState(stateObj).getNodeCount() << endl << endl;
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
      for(UnsignedSysInt i = 0; i < print_matrix.size(); ++i)
        for(UnsignedSysInt j = 0; j < print_matrix[i].size(); ++j)
        {
          if(!print_matrix[i][j].isAssigned())
            INPUT_ERROR("Some variable was unassigned while writing solution to file.");
          solsoutFile << print_matrix[i][j].getAssignedValue() << " ";
        }
      solsoutFile << "\n";
    }
    
    if(getOptions(stateObj).print_solution)
    {
      if(getOptions(stateObj).printonlyoptimal)
      {
        std::ostringstream oss;
        print_solution(stateObj, oss, getState(stateObj).getPrintMatrix());
        getState(stateObj).storedSolution = oss.str();
      }
      else
        print_solution(stateObj, cout, getState(stateObj).getPrintMatrix());
    }

    if(!getOptions(stateObj).nocheck)
    {
      for(UnsignedSysInt i = 0 ; i < getState(stateObj).getConstraintList().size();i++)
        check_constraint(stateObj, getState(stateObj).getConstraintList()[i]);
    }
  }

#include <fstream>
#include "../MILtools/print_CSP.h"

  //repeat declaration
  struct triple {
    bool isLeft;
    UnsignedSysInt var;
    DomainInt val;
    
    triple(bool _isLeft, UnsignedSysInt _var, DomainInt _val) : isLeft(_isLeft), var(_var), val(_val) {}
    friend std::ostream& operator<<(std::ostream& o, const triple& t)
    { o << "(" << t.isLeft << "," << t.var << "," << t.val << ")"; return o; }
  };

  template<typename VarArray, typename BranchList>
    inline void generateRestartFile(StateObj* stateObj, VarArray& var_array, BranchList& branches)
  {
    if(getOptions(stateObj).noresumefile) {
        return;
    }

    vector<string> splits;
    string curvar = "(no_split_variable)";
    string opt = "";

    if(getOptions(stateObj).split)
    {
        if(getState(stateObj).isOptimisationProblem()) {
            AnyVarRef *optVarRef = getState(stateObj).getOptimiseVar();
            string optVar = getState(stateObj).getInstance()->vars.getName(optVarRef->getBaseVar());
            DomainInt optVal = getState(stateObj).getOptimiseValue();
            opt += "ineq(";
            if(getState(stateObj).isMaximise()) {
                opt += to_string(optVal) + string(", ") + optVar + string(", 0)\n");
            } else {
                opt += optVar + string(", ") + to_string(-optVal) + string(", 0)\n");
            }
        }
        if(branches.empty())
        {
            // TODO: We should check if any variable has non-empty domain, but this will do for now.
            // The most likely case is we have just caught the end of search.
            splits.push_back("");
        }
        else
        {
            typedef typename VarArray::value_type VarRef;
            VarRef& var = var_array[branches.back().var];
            curvar = getState(stateObj).getInstance()->vars.getName(var.getBaseVar());
            DomainInt min = var.getMin();
            DomainInt max = var.getMax();
            DomainInt med = (min+max)/2;
            string left("ineq(");
            left += curvar + string(", ") + to_string(med) + string(", 0)\n");
            splits.push_back(left + opt);

            string right("ineq(");
            right += to_string(med) + string(", ");
            right += curvar + string(", -1)\n");
            splits.push_back(right + opt);
        }
    }
    else
    {
        splits.push_back("");
    }

    ProbSpec::MinionInstancePrinter printer(*getState(stateObj).getInstance());
    printer.build_instance(false);
    string inst(printer.getInstance());

    SysInt i = 0;
    for(vector<string>::iterator s = splits.begin(); s != splits.end(); s++) {
        if(! getOptions(stateObj).splitstderr) {
            string basename = getOptions(stateObj).instance_name;
            size_t mpos = basename.find(".minion");
            size_t rpos = basename.find("-resume-");
            if(rpos != string::npos) {
                basename = basename.substr(0, rpos);
            } else if(mpos != string::npos) {
                basename = basename.substr(0, mpos);
            }
            string filename = basename + "-resume-" + to_string(time(NULL)) + "-" + to_string(getpid()) + "-" + curvar + "-" + to_string(i++) + ".minion";
            cout << "Output resume file to \"" << filename << "\"" << endl;
            ofstream fileout(filename.c_str());
            fileout << "# original instance: " << getOptions(stateObj).instance_name << endl;
            fileout << inst;
            fileout << *s;
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
        else {
            //  For distributed use within BOINC, dump splits into stderr. Accessed by -split-stderr command-line flag. 
            cerr << "# original instance: " << getOptions(stateObj).instance_name << endl;
            cerr << inst;
            cerr << *s;
            vector<triple> left_branches_so_far;
            left_branches_so_far.reserve(branches.size());
            for(vector<triple>::const_iterator curr = branches.begin(); curr != branches.end(); curr++) {
              if(curr->isLeft) {
                left_branches_so_far.push_back(*curr);
              } else {
                cerr << "watched-or({";
                for(vector<triple>::const_iterator lb = left_branches_so_far.begin();
                    lb != left_branches_so_far.end();
                    lb++) {
                  cerr << "w-notliteral(";
                  inputPrint(cerr, stateObj, var_array[lb->var].getBaseVar());
                  cerr << "," << lb->val << "),";
                }
                cerr << "w-notliteral(";
                inputPrint(cerr, stateObj, var_array[curr->var].getBaseVar());
                cerr << "," << curr->val << ")})" << endl;
              }
            }
            cerr << "**EOF**" << endl;
        }
    }
  }
   
  /// Check if timelimit has been exceeded.
  template<typename VarArray, typename BranchList>
  inline void do_checks(StateObj* stateObj, VarArray& var_array, BranchList& branches)
  {
    if(getState(stateObj).getNodeCount() >= getOptions(stateObj).nodelimit) {
      generateRestartFile(stateObj, var_array, branches);
      throw EndOfSearch();
    }
    
    if(getState(stateObj).isAlarmActivated())
    { // Either a timeout has occurred, or ctrl+c has been pressed.
      generateRestartFile(stateObj, var_array, branches);
      getState(stateObj).clearAlarm();
      if(getState(stateObj).isCtrlcPressed()) {
        throw EndOfSearch();
      }

      if(getOptions(stateObj).cspcomp)
      {
        FAIL_EXIT("Time out");
      }
      
      getOptions(stateObj).printLine("Time out.");
      getTableOut().set("TimeOut", 1);

      throw EndOfSearch();
    }
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
      
      if(getOptions(stateObj).printonlyoptimal)
      {
        getState(stateObj).storedSolution += "Solution found with Value: "
          + to_string(getState(stateObj).getRawOptimiseVar()->getAssignedValue()) + "\n";
      }
      else
      {
        cout << "Solution found with Value: " 
             << getState(stateObj).getRawOptimiseVar()->getAssignedValue() << endl;
      }
      
      getState(stateObj).setOptimiseValue(getState(stateObj).getOptimiseVar()->getAssignedValue() + 1);         
    }
    // Note that sollimit = -1 if all solutions should be found.
    if(getState(stateObj).getSolutionCount() == getOptions(stateObj).sollimit)
      throw EndOfSearch();
  }

  template<typename Prop, typename Vars>
  void inline set_optimise_and_propagate_queue(StateObj* stateObj, Prop& propagator, Vars& vars)
  {
    if(getState(stateObj).isOptimisationProblem())
      getState(stateObj).getOptimiseVar()->setMin(getState(stateObj).getOptimiseValue());
    propagator.prop(stateObj, vars);
//    getQueue(stateObj).propagateQueue();
  }

  void inline initalise_search(StateObj* stateObj)
  {
    getState(stateObj).setSolutionCount(0);  
    getState(stateObj).setNodeCount(0);
    
    lock(stateObj);
    if (!getOptions(stateObj).silent) 
      getState(stateObj).getOldTimer().printTimestepWithoutReset(cout, Output_1, "First Node Time: ");
    /// Failed initially propagating constraints!
    if(getState(stateObj).isFailed())
      return;
    if(getState(stateObj).isOptimisationProblem())
      getState(stateObj).setOptimiseValue(getState(stateObj).getOptimiseVar()->getMin()); 
  }
}

#endif
