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

#include <sstream>

namespace ProbSpec {

struct MinionInstancePrinter {
  ostringstream oss;
  CSPInstance& csp;

  MinionInstancePrinter(CSPInstance& _csp) : csp(_csp) {}

  string getInstance() {
    return oss.str();
  }

#ifdef MINION_DEBUG
  void print_instance(const DomainInt& i) {
    oss << checked_cast<SysInt>(i);
  }
#endif

  void print_instance(const SysInt& i) {
    oss << i;
  }

  void print_instance(const string& s) {
    oss << s;
  }

  void print_instance(const Var& var) {
    if(var.type() == VAR_CONSTANT)
      print_instance(var.pos());
    else if(var.type() == VAR_NOTBOOL) {
      oss << "!";
      oss << csp.vars.getName(Var(VAR_BOOL, var.pos()));
    } else
      oss << csp.vars.getName(var);
  }

  template <typename T>
  void print_instance(const vector<T>& vars, char start = '[', char end = ']') {
    oss << start;
    if(!vars.empty()) {
      print_instance(vars[0]);
      for(SysInt i = 1; i < (SysInt)vars.size(); ++i) {
        oss << ",";
        print_instance(vars[i]);
      }
    }
    oss << end;
  }

  void print_instance(const ConstraintBlob& blob) {
    oss << blob.constraint->name;
    oss << "(";

    SysInt var_pos = 0;
    SysInt const_pos = 0;
    SysInt constraint_child_pos = 0;

    for(SysInt i = 0; i < blob.constraint->number_of_params; i++) {
      if(i != 0)
        oss << ", ";

      switch(blob.constraint->read_types[i]) {
      case read_list: print_instance(blob.vars[var_pos++]); break;
      case read_var: print_instance(blob.vars[var_pos++][0]); break;
      case read_2_vars: {
        print_instance(blob.vars[var_pos][0]);
        oss << ",";
        print_instance(blob.vars[var_pos++][1]);
      } break;
      case read_constant: print_instance(blob.constants[const_pos++][0]); break;
      case read_constant_list: print_instance(blob.constants[const_pos++]); break;
      case read_tuples: oss << blob.tuples->getName(); break;
      case read_short_tuples: oss << blob.shortTuples->getName(); break;
      case read_constraint:
        print_instance(blob.internal_constraints[constraint_child_pos]);
        constraint_child_pos++;
        break;
      case read_constraint_list:
        oss << "{";
        print_instance(blob.internal_constraints[0]);
        for(SysInt j = 1; j < (SysInt)blob.internal_constraints.size(); ++j) {
          oss << ", ";
          print_instance(blob.internal_constraints[j]);
        }
        oss << "}";
        break;
      default:
        oss << "???";
        //          D_FATAL_ERROR("Internal Error!");
      }
    }

    oss << ")";
    oss << endl;
  }

  void print_instance(const VarContainer& vars, const vector<Var>& varlist) {
    for(SysInt i = 0; i < (SysInt)varlist.size(); ++i) {
      switch(varlist[i].type()) {
      case VAR_BOOL:
        oss << "BOOL ";
        print_instance(varlist[i]);
        oss << endl;
        break;

      case VAR_BOUND: {
        oss << "BOUND ";
        print_instance(varlist[i]);
        pair<BoundType, vector<DomainInt>> bound = vars.getDomain(varlist[i]);
        D_ASSERT(bound.first == Bound_Yes);
        D_ASSERT((SysInt)bound.second.size() == 2);
        oss << "{" << bound.second[0] << ".." << bound.second[1] << "}" << endl;
      } break;

      case VAR_SPARSEBOUND: {
        oss << "SPARSEBOUND ";
        print_instance(varlist[i]);
        pair<BoundType, vector<DomainInt>> bound = vars.getDomain(varlist[i]);
        D_ASSERT(bound.first == Bound_No);
        print_instance(bound.second, '{', '}');
        oss << endl;
      } break;

      case VAR_DISCRETE: {
        oss << "DISCRETE ";
        print_instance(varlist[i]);
        pair<BoundType, vector<DomainInt>> bound = vars.getDomain(varlist[i]);
        D_ASSERT(bound.first == Bound_Yes);
        D_ASSERT((SysInt)bound.second.size() == 2);
        oss << "{" << bound.second[0] << ".." << bound.second[1] << "}" << endl;
      } break;

      default: abort();
      }
    }
  }

  void print_shortTuples() {
    typedef map<string, ShortTupleList*>::const_iterator it_type;

    for(it_type it = csp.shorttable_symboltable.begin(); it != csp.shorttable_symboltable.end();
        ++it) {
      oss << it->first << " " << it->second->size() << "\n";

      vector<vector<pair<SysInt, DomainInt>>> const* tptr = it->second->tuplePtr();
      for(int i = 0; i < tptr->size(); ++i) {
        const vector<pair<SysInt, DomainInt>>& tup = (*tptr)[i];
        oss << "[";
        bool first = true;
        for(int j = 0; j < tup.size(); ++j) {
          if(first) {
            first = false;
          } else {
            oss << ", ";
          }
          oss << "(" << tup[j].first << "," << tup[j].second << ")";
        }

        oss << "]\n";
      }
      oss << endl;
    }
  }

  void print_tuples() {
    typedef map<string, TupleList*>::const_iterator it_type;

    for(it_type it = csp.table_symboltable.begin(); it != csp.table_symboltable.end(); ++it) {
      oss << it->first << " ";
      DomainInt tupleSize = it->second->tupleSize();
      DomainInt num_tuples = it->second->size();
      DomainInt* tuplePtr = it->second->getPointer();
      oss << num_tuples << " " << tupleSize << endl;
      for(DomainInt i = 0; i < num_tuples; ++i) {
        for(DomainInt j = 0; j < tupleSize; ++j)
          oss << *(tuplePtr + checked_cast<SysInt>((i * tupleSize) + j)) << " ";
        oss << endl;
      }
      oss << endl;
    }
  }

  void printSearchInfo(const vector<Var>& varVec) {
    set<Var> vars(varVec.begin(), varVec.end());

    if(csp.is_optimisation_problem && vars.count(csp.optimiseVariable)) {
      if(csp.optimise_minimising)
        oss << "MINIMISING ";
      else
        oss << "MAXIMISING ";
      print_instance(csp.optimiseVariable);
      oss << endl;
    }

    for(SysInt i = 0; i < (SysInt)csp.searchOrder.size(); ++i) {
      // Filter the var and val orders.

      vector<Var> varOrder = csp.searchOrder[i].varOrder;
      vector<ValOrder> valOrder = csp.searchOrder[i].valOrder;

      SysInt pos = 0;
      while(pos < (SysInt)varOrder.size()) {
        if(vars.count(varOrder[pos]) == 0) {
          varOrder.erase(varOrder.begin() + pos);
          valOrder.erase(valOrder.begin() + pos);
        } else
          pos++;
      }

      if(!varOrder.empty()) {
        oss << "VARORDER ";
        if(csp.searchOrder[i].find_one_assignment) {
          oss << "AUX ";
        }

        if(csp.searchOrder[i].order != ORDER_ORIGINAL) {
          oss << csp.searchOrder[i].order << " ";
        }

        print_instance(varOrder);
        oss << endl;
      }

      if(!valOrder.empty()) {
        oss << "VALORDER ";
        vector<string> output_vars;
        for(SysInt j = 0; j < (SysInt)valOrder.size(); ++j)
          switch(valOrder[j].type) {
          case VALORDER_NONE: D_FATAL_ERROR("Invalid value ordering");
          case VALORDER_ASCEND: output_vars.push_back("a"); break;
          case VALORDER_DESCEND: output_vars.push_back("d"); break;
          case VALORDER_RANDOM: output_vars.push_back("r"); break;
          }
        print_instance(output_vars);
        oss << endl;
      }
    }
    if(!csp.permutation.empty()) {
      oss << "PERMUTATION ";
      print_instance(csp.permutation);
      oss << endl;
    }

    if(!csp.sym_order.empty()) {
      oss << "SYMORDER ";
      print_instance(csp.sym_order);
      oss << endl;
    }
    if(csp.print_matrix.empty()) {
      oss << "PRINT NONE" << endl;
    } else {
      vector<vector<Var>> new_print_matrix;
      for(SysInt i = 0; i < (SysInt)csp.print_matrix.size(); ++i) {
        new_print_matrix.push_back(vector<Var>());
        for(SysInt j = 0; j < (SysInt)csp.print_matrix[i].size(); ++j) {
          if(vars.count(csp.print_matrix[i][j]))
            new_print_matrix[i].push_back(csp.print_matrix[i][j]);
        }
      }

      oss << "PRINT";
      print_instance(new_print_matrix);
      oss << endl;
    }
  }

  void buildInstance() {
    buildInstance(csp.constraints, csp.vars.getAllVars(), true);
  }

  void buildInstance(bool printEof) {
    buildInstance(csp.constraints, csp.vars.getAllVars(), printEof);
  }

  void buildInstance(const vector<Var>& varlistVec, bool printEof) {
    list<ConstraintBlob> new_constraint_list;

    set<Var> varlist(varlistVec.begin(), varlistVec.end());

    // set<Var> list_of_vars
    for(list<ConstraintBlob>::iterator it = csp.constraints.begin(); it != csp.constraints.end();
        ++it) {
      set<Var> vars = it->getAllVars();
      if(includes(varlist.begin(), varlist.end(), vars.begin(), vars.end()))
        new_constraint_list.push_back(*it);
    }

    buildInstance(new_constraint_list, varlistVec, printEof);
  }

  void buildInstance(const list<ConstraintBlob>& constraints, const vector<Var>& varlist,
                      bool printEof) {
    oss << "MINION 3" << endl;

    csp.add_variable_names();

    oss << "**VARIABLES**" << endl;
    print_instance(csp.vars, varlist);

    oss << "**SEARCH**" << endl;
    printSearchInfo(varlist);

    oss << "**TUPLELIST**" << endl;
    print_tuples();

    oss << "**SHORTTUPLELIST**" << endl;
    print_shortTuples();

    oss << "**CONSTRAINTS**" << endl;
    for(list<ConstraintBlob>::const_iterator it = constraints.begin(); it != constraints.end();
        ++it) {
      print_instance(*it);
    }
    if(printEof)
      oss << "**EOF**" << endl;
  }
};
} // namespace ProbSpec
