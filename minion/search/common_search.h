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

#ifndef COMMON_SEARCH_H
#define COMMON_SEARCH_H

#include "../system/system.h"
#include "../solver.h"
#include "../variables/AnyVarRef.h"
#include "../variables/mappings/variable_neg.h"

namespace Controller {

/// Sets optimisation variable.
template <typename VarRef>
void optimise_maximise_var(VarRef var) {
  getOptions().findAllSolutions();
  getState().setOptimiseVar(new AnyVarRef(var));
  getState().setOptimisationProblem(true);
  getState().setRawOptimiseVar(new AnyVarRef(var));
  getState().setMaximise(true);
}

/// Sets optimisation variable.
template <typename VarRef>
void optimise_minimise_var(VarRef var) {
  getOptions().findAllSolutions();
  getState().setOptimiseVar(new AnyVarRef(VarNeg<VarRef>(var)));
  getState().setOptimisationProblem(true);
  getState().setRawOptimiseVar(new AnyVarRef(var));
  getState().setMaximise(false);
}

/// Ensures a particular constraint is satisfied by the solution.
template <typename T>
void check_constraint(T* con) {
  vector<AnyVarRef>& variables = *(con->get_vars_singleton());
  UnsignedSysInt vec_size = variables.size();

  DomainInt* values = (DomainInt*)alloca(vec_size * sizeof(DomainInt));
  // vector<DomainInt> values(vec_size);

  for(UnsignedSysInt loop = 0; loop < vec_size; ++loop) {
    if(!variables[loop].isAssigned()) {
      cerr << "Some variables are unassigned. Unless you purposefully "
           << "left them out, have a look." << endl;
      return;
    }
    values[loop] = variables[loop].getAssignedValue();
  }

  if(!con->check_assignment(values, vec_size)) {
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

template <typename Stream, typename PrintMatrix>
void print_solution(Stream& sout, const PrintMatrix& print_matrix) {
  if(!print_matrix.empty()) {
    for(UnsignedSysInt i = 0; i < print_matrix.size(); ++i) {
      if(!getOptions().silent)
        sout << "Sol: ";
      for(UnsignedSysInt j = 0; j < print_matrix[i].size(); ++j) {
        if(!print_matrix[i][j].isAssigned())
          sout << "[" << print_matrix[i][j].getMin() << "," << print_matrix[i][j].getMax() << "]";
        else
          sout << print_matrix[i][j].getAssignedValue() << " ";
      }
      sout << endl;
    }
    if(!getOptions().silent)
      sout << endl;
  }

  // TODO : Make this more easily changable.
  if(!getOptions().silent) {
    sout << "Solution Number: " << getState().getSolutionCount() << endl;
    getState().getOldTimer().printTimestepWithoutReset(sout, "Time:");
    sout << "Nodes: " << getState().getNodeCount() << endl << endl;
  }
}

/// All operations to be performed when a solution is found.
/// This function checks the solution is correct, and prints it if required.
inline void check_sol_is_correct() {
  if(getOptions().solCallBack)
    getOptions().solCallBack();

  getState().incrementSolutionCount();

  if(getOptions().solsoutWrite) {
    vector<vector<AnyVarRef>> print_matrix = getState().getPrintMatrix();
    for(UnsignedSysInt i = 0; i < print_matrix.size(); ++i)
      for(UnsignedSysInt j = 0; j < print_matrix[i].size(); ++j) {
        if(!print_matrix[i][j].isAssigned())
          INPUT_ERROR("Some variable was unassigned while writing solution to file.");
        solsoutFile << print_matrix[i][j].getAssignedValue() << " ";
      }
    solsoutFile << "\n";
  }

  if(getOptions().print_solution) {
    if(getOptions().printonlyoptimal) {
      std::ostringstream oss;
      print_solution(oss, getState().getPrintMatrix());
      getState().storedSolution = oss.str();
    } else
      print_solution(cout, getState().getPrintMatrix());
  }

  if(!getOptions().nocheck) {
    for(UnsignedSysInt i = 0; i < getState().getConstraintList().size(); i++)
      check_constraint(getState().getConstraintList()[i]);
  }
}

#include <fstream>
#include "../MILtools/print_CSP.h"

// repeat declaration
struct triple {
  bool isLeft;
  SysInt var;
  DomainInt val;

  triple(bool _isLeft, SysInt _var, DomainInt _val) : isLeft(_isLeft), var(_var), val(_val) {}
  friend std::ostream& operator<<(std::ostream& o, const triple& t) {
    o << "(" << t.isLeft << "," << t.var << "," << t.val << ")";
    return o;
  }
};

template <typename VarArray, typename BranchList>
inline void generateRestartFile(VarArray& var_array, BranchList& branches) {
  if(getOptions().noresumefile) {
    return;
  }

  vector<string> splits;
  string curvar = "(no_split_variable)";
  string opt = "";

  if(getOptions().split) {
    if(getState().isOptimisationProblem()) {
      AnyVarRef* optVarRef = getState().getOptimiseVar();
      string optVar = getState().getInstance()->vars.getName(optVarRef->getBaseVar());
      DomainInt optVal = getState().getOptimiseValue();
      opt += "ineq(";
      if(getState().isMaximise()) {
        opt += tostring(optVal) + string(", ") + optVar + string(", 0)\n");
      } else {
        opt += optVar + string(", ") + tostring(-optVal) + string(", 0)\n");
      }
    }
    if(branches.empty()) {
      // TODO: We should check if any variable has non-empty domain, but this
      // will do for now.
      // The most likely case is we have just caught the end of search.
      splits.push_back("");
    } else {
      typedef typename VarArray::value_type VarRef;
      VarRef& var = var_array[branches.back().var];
      curvar = getState().getInstance()->vars.getName(var.getBaseVar());
      DomainInt min = var.getMin();
      DomainInt max = var.getMax();
      DomainInt med = (min + max) / 2;
      string left("ineq(");
      left += curvar + string(", ") + tostring(med) + string(", 0)\n");
      splits.push_back(left + opt);

      string right("ineq(");
      right += tostring(med) + string(", ");
      right += curvar + string(", -1)\n");
      splits.push_back(right + opt);
    }
  } else {
    splits.push_back("");
  }

  ProbSpec::MinionInstancePrinter printer(*getState().getInstance());
  printer.build_instance(false);
  string inst(printer.getInstance());

  SysInt i = 0;
  for(vector<string>::iterator s = splits.begin(); s != splits.end(); s++) {
    if(!getOptions().splitstderr) {
      string basename = getOptions().instance_name;
      size_t mpos = basename.find(".minion");
      size_t rpos = basename.find("-resume-");
      if(rpos != string::npos) {
        basename = basename.substr(0, rpos);
      } else if(mpos != string::npos) {
        basename = basename.substr(0, mpos);
      }
      string filename = basename + "-resume-" + tostring(time(NULL)) + "-" + tostring(getpid()) +
                        "-" + curvar + "-" + tostring(i++) + ".minion";
      cout << "Output resume file to \"" << filename << "\"" << endl;
      ofstream fileout(filename.c_str());
      fileout << "# original instance: " << getOptions().instance_name << endl;
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
              lb != left_branches_so_far.end(); lb++) {
            fileout << "w-notliteral(";
            inputPrint(fileout, var_array[lb->var].getBaseVar());
            fileout << "," << lb->val << "),";
          }
          fileout << "w-notliteral(";
          inputPrint(fileout, var_array[curr->var].getBaseVar());
          fileout << "," << curr->val << ")})" << endl;
        }
      }
      fileout << "**EOF**" << endl;
    } else {
      //  For distributed use within BOINC, dump splits into stderr. Accessed by
      //  -split-stderr command-line flag.
      cerr << "# original instance: " << getOptions().instance_name << endl;
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
              lb != left_branches_so_far.end(); lb++) {
            cerr << "w-notliteral(";
            inputPrint(cerr, var_array[lb->var].getBaseVar());
            cerr << "," << lb->val << "),";
          }
          cerr << "w-notliteral(";
          inputPrint(cerr, var_array[curr->var].getBaseVar());
          cerr << "," << curr->val << ")})" << endl;
        }
      }
      cerr << "**EOF**" << endl;
    }
  }
}

/// Check if timelimit has been exceeded.
template <typename VarArray, typename BranchList>
inline void do_checks(VarArray& var_array, BranchList& branches) {
  if(getState().getNodeCount() >= getOptions().nodelimit) {
    generateRestartFile(var_array, branches);
    throw EndOfSearch();
  }

  if(getState().isAlarmActivated()) { // Either a timeout has occurred, or
                                      // ctrl+c has been pressed.
    generateRestartFile(var_array, branches);
    getState().clearAlarm();
    if(getState().isCtrlcPressed()) {
      throw EndOfSearch();
    }

    getOptions().printLine("Time out.");
    getTableOut().set("TimeOut", 1);

    throw EndOfSearch();
  }
}

template <typename T>
void inline maybe_print_node(T& vars) {
  if(getOptions().dumptree)
    cout << "Node: " << getState().getNodeCount() << "," << get_dom_as_string(vars) << endl;
  if(getOptions().dumpjsontree.isActive()) {
    getOptions().dumpjsontree.mapElement("Node", getState().getNodeCount());
    getOptions().dumpjsontree.mapElement("Domains", get_dom_as_json(vars));
    getOptions().dumpjsontree.newline();
  }
}

void inline maybe_print_backtrack() {
  // used to print "bt" usually
  if(getOptions().dumptree)
    cout << "SearchAction: bt" << endl;
  if(getOptions().dumpjsontree.isActive()) {
    getOptions().dumpjsontree.closeMap();
  }
}

// This is a seperate method, as we don't print right backtracks
// in dumptree, for historical reasons
void inline maybe_print_right_backtrack() {
  // used to print "bt" usually
  if(getOptions().dumpjsontree.isActive()) {
    getOptions().dumpjsontree.closeMap();
  }
}

void inline deal_with_solution() {
  // We have found a solution!
  check_sol_is_correct();

  if(getState().isOptimisationProblem()) {
    if(!getState().getOptimiseVar()->isAssigned()) {
      cerr << "The optimisation variable isn't assigned at a solution node!" << endl;
      cerr << "Put it in the variable ordering?" << endl;
      cerr << "Aborting Search" << endl;
      exit(1);
    }

    if(getOptions().printonlyoptimal) {
      getState().storedSolution += "Solution found with Value: " +
                                   tostring(getState().getRawOptimiseVar()->getAssignedValue()) +
                                   "\n";
    } else {
      cout << "Solution found with Value: " << getState().getRawOptimiseVar()->getAssignedValue()
           << endl;
    }

    getState().setOptimiseValue(getState().getOptimiseVar()->getAssignedValue() + 1);
  }
  // Note that sollimit = -1 if all solutions should be found.
  if(getState().getSolutionCount() == getOptions().sollimit)
    throw EndOfSearch();
}

void inline initalise_search() {
  getState().setSolutionCount(0);
  getState().setNodeCount(0);

  lock();
  /// Failed initially propagating constraints!
  if(getState().isFailed())
    return;
  if(getState().isOptimisationProblem())
    getState().setOptimiseValue(getState().getOptimiseVar()->getMin());
}
}

#endif
