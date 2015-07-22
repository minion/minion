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
  CSPInstance &csp;

  MinionInstancePrinter(CSPInstance &_csp) : csp(_csp) {}

  string getInstance() { return oss.str(); }

#ifdef MINION_DEBUG
  void print_instance(const DomainInt &i) { oss << checked_cast<SysInt>(i); }
#endif

  void print_instance(const SysInt &i) { oss << i; }

  void print_instance(const string &s) { oss << s; }

  void print_instance(const Var &var) {
    if (var.type() == VAR_CONSTANT)
      print_instance(var.pos());
    else if (var.type() == VAR_NOTBOOL) {
      oss << "!";
      oss << csp.vars.getName(Var(VAR_BOOL, var.pos()));
    } else
      oss << csp.vars.getName(var);
  }

  template <typename T>
  void print_instance(const vector<T> &vars, char start = '[', char end = ']') {
    oss << start;
    if (!vars.empty()) {
      print_instance(vars[0]);
      for (SysInt i = 1; i < (SysInt)vars.size(); ++i) {
        oss << ",";
        print_instance(vars[i]);
      }
    }
    oss << end;
  }

  void print_instance(const ConstraintBlob &blob) {
    oss << blob.constraint->name;
    oss << "(";

    SysInt var_pos = 0;
    SysInt const_pos = 0;
    SysInt constraint_child_pos = 0;

    for (SysInt i = 0; i < blob.constraint->number_of_params; i++) {
      if (i != 0)
        oss << ", ";

      switch (blob.constraint->read_types[i]) {
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
      case read_short_tuples: oss << blob.short_tuples->getName(); break;
      case read_constraint:
        print_instance(blob.internal_constraints[constraint_child_pos]);
        constraint_child_pos++;
        break;
      case read_constraint_list:
        oss << "{";
        print_instance(blob.internal_constraints[0]);
        for (SysInt j = 1; j < (SysInt)blob.internal_constraints.size(); ++j) {
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

  void print_instance(const VarContainer &vars, const vector<Var> &varlist) {
    for (SysInt i = 0; i < (SysInt)varlist.size(); ++i) {
      switch (varlist[i].type()) {
      case VAR_BOOL:
        oss << "BOOL ";
        print_instance(varlist[i]);
        oss << endl;
        break;

      case VAR_BOUND: {
        oss << "BOUND ";
        print_instance(varlist[i]);
        pair<BoundType, vector<DomainInt>> bound = vars.get_domain(varlist[i]);
        D_ASSERT(bound.first == Bound_Yes);
        D_ASSERT((SysInt)bound.second.size() == 2);
        oss << "{" << bound.second[0] << ".." << bound.second[1] << "}" << endl;
      } break;

      case VAR_SPARSEBOUND: {
        oss << "SPARSEBOUND ";
        print_instance(varlist[i]);
        pair<BoundType, vector<DomainInt>> bound = vars.get_domain(varlist[i]);
        D_ASSERT(bound.first == Bound_No);
        print_instance(bound.second, '{', '}');
        oss << endl;
      } break;

      case VAR_DISCRETE: {
        oss << "DISCRETE ";
        print_instance(varlist[i]);
        pair<BoundType, vector<DomainInt>> bound = vars.get_domain(varlist[i]);
        D_ASSERT(bound.first == Bound_Yes);
        D_ASSERT((SysInt)bound.second.size() == 2);
        oss << "{" << bound.second[0] << ".." << bound.second[1] << "}" << endl;
      } break;

      default: abort();
      }
    }

    return;

    for (SysInt i = 0; i < vars.BOOLs; ++i) {
      oss << "BOOL ";
      print_instance(Var(VAR_BOOL, i));
      oss << endl;
    }

    // Bounds.
    SysInt bound_sum = 0;
    for (SysInt x = 0; x < (SysInt)vars.bound.size(); ++x) {
      for (SysInt i = 0; i < vars.bound[x].first; ++i) {
        oss << "BOUND ";
        print_instance(Var(VAR_BOUND, i + bound_sum));
        oss << "{" << vars.bound[x].second.lower_bound << ".." << vars.bound[x].second.upper_bound
            << "}" << endl;
      }
      bound_sum += vars.bound[x].first;
    }

    // Sparse Bounds.

    SysInt sparse_bound_sum = 0;
    for (SysInt x = 0; x < (SysInt)vars.sparse_bound.size(); ++x) {
      for (SysInt i = 0; i < vars.sparse_bound[x].first; ++i) {
        oss << "SPARSEBOUND ";
        print_instance(Var(VAR_BOUND, i + sparse_bound_sum));
        oss << " ";
        print_instance(vars.sparse_bound[x].second, '{', '}');
        oss << endl;
      }
      sparse_bound_sum += vars.sparse_bound[x].first;
    }

    // Bounds.
    SysInt discrete_sum = 0;
    for (SysInt x = 0; x < (SysInt)vars.discrete.size(); ++x) {
      for (SysInt i = 0; i < vars.discrete[x].first; ++i) {
        oss << "DISCRETE ";
        print_instance(Var(VAR_DISCRETE, i + discrete_sum));
        oss << "{" << vars.discrete[x].second.lower_bound << ".."
            << vars.discrete[x].second.upper_bound << "}" << endl;
      }
      discrete_sum += vars.discrete[x].first;
    }
  }

  void print_tuples() {
    typedef map<string, TupleList *>::const_iterator it_type;

    for (it_type it = csp.table_symboltable.begin(); it != csp.table_symboltable.end(); ++it) {
      oss << it->first << " ";
      DomainInt tuple_size = it->second->tuple_size();
      DomainInt num_tuples = it->second->size();
      DomainInt *tuple_ptr = it->second->getPointer();
      oss << num_tuples << " " << tuple_size << endl;
      for (DomainInt i = 0; i < num_tuples; ++i) {
        for (DomainInt j = 0; j < tuple_size; ++j)
          oss << *(tuple_ptr + checked_cast<SysInt>((i * tuple_size) + j)) << " ";
        oss << endl;
      }
      oss << endl;
    }
  }

  void print_search_info(const vector<Var> &var_vec) {
    set<Var> vars(var_vec.begin(), var_vec.end());

    if (csp.is_optimisation_problem && vars.count(csp.optimise_variable)) {
      if (csp.optimise_minimising)
        oss << "MINIMISING ";
      else
        oss << "MAXIMISING ";
      print_instance(csp.optimise_variable);
      oss << endl;
    }

    for (SysInt i = 0; i < (SysInt)csp.search_order.size(); ++i) {
      // Filter the var and val orders.

      vector<Var> var_order = csp.search_order[i].var_order;
      vector<ValOrderEnum> val_order = csp.search_order[i].val_order;

      SysInt pos = 0;
      while (pos < (SysInt)var_order.size()) {
        if (vars.count(var_order[pos]) == 0) {
          var_order.erase(var_order.begin() + pos);
          val_order.erase(val_order.begin() + pos);
        } else
          pos++;
      }

      if (!var_order.empty()) {
        oss << "VARORDER ";
        print_instance(var_order);
        oss << endl;
      }

      if (!val_order.empty()) {
        oss << "VALORDER ";
        vector<string> output_vars;
        for (SysInt j = 0; j < (SysInt)val_order.size(); ++j)
          switch (val_order[j]) {
          case VALORDER_ASCEND: output_vars.push_back("a"); break;
          case VALORDER_DESCEND: output_vars.push_back("d"); break;
          case VALORDER_RANDOM: output_vars.push_back("r"); break;
          }
        print_instance(output_vars);
        oss << endl;
      }
    }
    if (!csp.permutation.empty()) {
      oss << "PERMUTATION ";
      print_instance(csp.permutation);
      oss << endl;
    }

    if (!csp.sym_order.empty()) {
      oss << "SYMORDER ";
      print_instance(csp.sym_order);
      oss << endl;
    }
    if (csp.print_matrix.empty()) {
      oss << "PRINT NONE" << endl;
    } else {
      vector<vector<Var>> new_print_matrix;
      for (SysInt i = 0; i < (SysInt)csp.print_matrix.size(); ++i) {
        new_print_matrix.push_back(vector<Var>());
        for (SysInt j = 0; j < (SysInt)csp.print_matrix[i].size(); ++j) {
          if (vars.count(csp.print_matrix[i][j]))
            new_print_matrix[i].push_back(csp.print_matrix[i][j]);
        }
      }

      oss << "PRINT";
      print_instance(new_print_matrix);
      oss << endl;
    }
  }

  void build_instance() { build_instance(csp.constraints, csp.vars.get_all_vars(), true); }

  void build_instance(bool printEof) {
    build_instance(csp.constraints, csp.vars.get_all_vars(), printEof);
  }

  void build_instance(const vector<Var> &varlist_vec, bool printEof) {
    list<ConstraintBlob> new_constraint_list;

    set<Var> varlist(varlist_vec.begin(), varlist_vec.end());

    // set<Var> list_of_vars
    for (list<ConstraintBlob>::iterator it = csp.constraints.begin(); it != csp.constraints.end();
         ++it) {
      set<Var> vars = it->get_all_vars();
      if (includes(varlist.begin(), varlist.end(), vars.begin(), vars.end()))
        new_constraint_list.push_back(*it);
    }

    build_instance(new_constraint_list, varlist_vec, printEof);
  }

  void build_instance(const list<ConstraintBlob> &constraints, const vector<Var> &varlist,
                      bool printEof) {
    oss << "MINION 3" << endl;

    csp.add_variable_names();

    oss << "**VARIABLES**" << endl;
    print_instance(csp.vars, varlist);

    oss << "**SEARCH**" << endl;
    print_search_info(varlist);

    oss << "**TUPLELIST**" << endl;
    print_tuples();

    oss << "**CONSTRAINTS**" << endl;
    for (list<ConstraintBlob>::const_iterator it = constraints.begin(); it != constraints.end();
         ++it) {
      print_instance(*it);
    }
    if (printEof)
      oss << "**EOF**" << endl;
  }
};
}
